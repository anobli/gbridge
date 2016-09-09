/*
 * GBridge (Greybus Bridge)
 * Copyright (c) 2016 Alexandre Bailon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <debug.h>
#include <gb_netlink.h>

#include "gbridge.h"
#include "netlink.h"

static greybus_handler_t gb_handlers[GB_NETLINK_NUM_CPORT];
static TAILQ_HEAD(head, operation) operations;

enum gb_operation_result {
	GB_OP_SUCCESS		= 0x00,
	GB_OP_INTERRUPTED	= 0x01,
	GB_OP_TIMEOUT		= 0x02,
	GB_OP_NO_MEMORY		= 0x03,
	GB_OP_PROTOCOL_BAD	= 0x04,
	GB_OP_OVERFLOW		= 0x05,
	GB_OP_INVALID		= 0x06,
	GB_OP_RETRY		= 0x07,
	GB_OP_NONEXISTENT	= 0x08,
	GB_OP_UNKNOWN_ERROR	= 0xfe,
	GB_OP_INTERNAL		= 0xff,
};

uint8_t greybus_errno_to_result(int err)
{
	switch (err) {
	case 0:
		return GB_OP_SUCCESS;
	case -ENOMEM:
		return GB_OP_NO_MEMORY;
	case -EINTR:
		return GB_OP_INTERRUPTED;
	case -ETIMEDOUT:
		return GB_OP_TIMEOUT;
	case -EPROTO:
	case -ENOSYS:
		return GB_OP_PROTOCOL_BAD;
	case -EINVAL:
		return GB_OP_INVALID;
	case -EOVERFLOW:
		return GB_OP_OVERFLOW;
	case -ENODEV:
	case -ENXIO:
		return GB_OP_NONEXISTENT;
	case -EBUSY:
		return GB_OP_RETRY;
	default:
		return GB_OP_UNKNOWN_ERROR;
	}
}

static struct operation *_greybus_alloc_operation(struct gb_operation_msg_hdr
						  *hdr)
{
	struct operation *op;

	op = malloc(sizeof(*op));
	if (!op)
		return NULL;

	op->req = malloc(gb_operation_msg_size(hdr));
	if (!op->req) {
		free(op);
		return NULL;
	}

	op->resp = NULL;
	memcpy(op->req, hdr, gb_operation_msg_size(hdr));

	return op;
}

struct operation *greybus_alloc_operation(uint8_t type,
					  void *payload, size_t len)
{
	uint16_t id = 0;
	struct operation *op;

	op = malloc(sizeof(*op));
	if (!op)
		return NULL;

	op->req = malloc(sizeof(*op->req) + len);
	if (!op->req) {
		free(op);
		return NULL;
	}

	if (++id == 0)
		++id;

	op->resp = NULL;
	op->req->type = type;
	op->req->size = htole16(sizeof(*op->req) + len);
	op->req->operation_id = htole16(id);
	memcpy(op->req + 1, payload, len);

	return op;
}

static int _greybus_alloc_response(struct operation *op,
				   struct gb_operation_msg_hdr *hdr)
{
	op->resp = malloc(gb_operation_msg_size(hdr));
	if (!op->resp)
		return -ENOMEM;

	memcpy(op->resp, hdr, gb_operation_msg_size(hdr));

	return 0;
}

int greybus_alloc_response(struct operation *op, size_t size)
{
	size += sizeof(*op->resp);
	if (size > GB_NETLINK_MTU)
		return -EINVAL;

	op->resp = malloc(size);
	if (!op->resp)
		return -ENOMEM;

	op->resp->operation_id = op->req->operation_id;
	op->resp->size = size;
	op->resp->type = op->req->type | 0x80;

	return 0;
}

static void greybus_free_operation(struct operation *op)
{
	free(op->req);
	free(op->resp);
	free(op);
}

int greybus_send_request(uint16_t cport_id, struct operation *op)
{
	int len;
	int ret;

	len = gb_operation_msg_size(op->req);
	pr_dump(op->req, len);

	op->cport_id = cport_id;
	TAILQ_INSERT_TAIL(&operations, op, cnode);
	ret = netlink_send(cport_id, op->req, len);
	if (ret < 0)
		return ret;

	return 0;
}

static int greybus_send_response(uint16_t cport_id, struct operation *op)
{
	int len;
	int ret;

	len = gb_operation_msg_size(op->resp);
	pr_dump(op->resp, len);

	ret = netlink_send(cport_id, op->resp, len);
	if (ret < 0)
		return ret;

	return 0;
}

struct operation *greybus_find_operation(uint16_t cport_id, uint8_t id)
{
	struct operation *op;
	TAILQ_FOREACH(op, &operations, cnode) {
		if (op->req->operation_id == id) {
			if (op->cport_id == cport_id) {
				return op;
			}
		}
	}

	return NULL;
}

int greybus_handler(uint16_t cport_id, struct gb_operation_msg_hdr *hdr)
{
	int ret;
	struct operation *op;

	pr_dump(hdr, gb_operation_msg_size(hdr));

	if (!gb_handlers[cport_id]) {
		pr_err("No driver registered for cport %d\n", cport_id);
		return -EINVAL;
	}

	if (hdr->type & OP_RESPONSE) {
		op = greybus_find_operation(cport_id, hdr->operation_id);
		if (!op) {
			pr_err("Invalid response id %d on cport %d\n",
			       le16toh(hdr->operation_id), cport_id);
			return -EINVAL;
		}
		TAILQ_REMOVE(&operations, op, cnode);
		if (_greybus_alloc_response(op, hdr))
			return -ENOMEM;
		ret = (gb_handlers[cport_id])(op);
	} else {
		op = _greybus_alloc_operation(hdr);
		if (!op)
			return -ENOMEM;

		ret = (gb_handlers[cport_id])(op);
		if (!op->resp) {
			if (greybus_alloc_response(op, 0)) {
				pr_err("Failed to alloc greybus response\n");
				ret = -ENOMEM;
				goto free_op;
			}
		}
		op->resp->result = greybus_errno_to_result(ret);

		ret = greybus_send_response(cport_id, op);
	}

 free_op:
	greybus_free_operation(op);

	return ret;
}

int greybus_register_driver(uint16_t cport_id, greybus_handler_t handler)
{
	if (cport_id >= GB_NETLINK_NUM_CPORT) {
		pr_err("Invalid cport id %d\n", cport_id);
		return -EINVAL;
	}
	gb_handlers[cport_id] = handler;

	return 0;
}

int greybus_init(void)
{
	TAILQ_INIT(&operations);
	memset(gb_handlers, 0,
	       sizeof(greybus_handler_t) * GB_NETLINK_NUM_CPORT);

	return greybus_register_driver(0, svc_handler);
}
