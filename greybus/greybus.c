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
#include <greybus/libgreybus.h>

static TAILQ_HEAD(operation_head, operation) operations;
static struct greybus_platform *g_platform;

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
	if (size > GREYBUS_MTU)
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

int greybus_send_request(uint8_t intf_id, uint16_t cport_id,
			 struct operation *op)
{
	int len;
	int ret;

	len = gb_operation_msg_size(op->req);
	pr_dump(op->req, len);

	op->intf_id = intf_id;
	op->cport_id = cport_id;
	TAILQ_INSERT_TAIL(&operations, op, cnode);
	ret = g_platform->greybus_send_request(intf_id, cport_id, op->req, len);
	if (ret < 0)
		return ret;

	return 0;
}

static int greybus_send_response(uint8_t intf_id, uint16_t cport_id,
				 struct operation *op)
{
	int len;
	int ret;

	len = gb_operation_msg_size(op->resp);
	pr_dump(op->resp, len);

	ret = g_platform->greybus_send_response(intf_id, cport_id,
						op->resp, len);
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

int compare_operation(const void *a, const void *b)
{
	const struct operation_handler *handler_a = a;
	const struct operation_handler *handler_b = b;

	return (handler_a->id > handler_b->id) -
		(handler_a->id < handler_b->id);
}

int _greybus_handler(struct greybus_driver *driver, struct operation *op)
{
	struct operation_handler key;
	struct operation_handler *handler;

	if (op->resp)
		key.id = op->resp->type;
	else
		key.id = op->req->type;
	handler = bsearch(&key, driver->operations, driver->count,
			  sizeof(struct operation_handler), compare_operation);
	if (!handler) {
		pr_err("No handler registered for operation type 0x%02x"
			" in %s driver\n", op->req->type, driver->name);
		return -ENOENT;
	}

	if (!handler->callback) {
		pr_err("No supported operation type 0x%02x in %s driver\n",
			op->req->type, driver->name);
		return -EOPNOTSUPP;
	}

	return handler->callback(op);
}

int greybus_handler(uint8_t intf2_id, uint16_t cport_id,
		    struct gb_operation_msg_hdr *hdr)
{
	int ret;
	struct operation *op;
	struct greybus_driver *driver;

	pr_dump(hdr, gb_operation_msg_size(hdr));

	driver = g_platform->greybus_driver(intf2_id, cport_id);
	if (!driver) {
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

		ret = _greybus_handler(driver, op);
	} else {
		op = _greybus_alloc_operation(hdr);
		if (!op)
			return -ENOMEM;

		op->intf_id = intf2_id;
		op->cport_id = cport_id;
		ret = _greybus_handler(driver, op);
		if (!op->resp) {
			if (greybus_alloc_response(op, 0)) {
				pr_err("Failed to alloc greybus response\n");
				ret = -ENOMEM;
				goto free_op;
			}
		}
		op->resp->result = greybus_errno_to_result(ret);

		ret = greybus_send_response(intf2_id, cport_id, op);
	}

 free_op:
	greybus_free_operation(op);

	return ret;
}

int greybus_register_driver(uint8_t intf_id, uint16_t cport_id,
			    struct greybus_driver *driver)
{
	int i = 0;
	int id = 0;
	int id_min = -1;

	if (cport_id >= GREYBUS_NUM_CPORT) {
		pr_err("Invalid cport id %d\n", cport_id);
		return -EINVAL;
	}

	if (g_platform->greybus_driver(intf_id, cport_id)) {
		pr_err("A driver has already been registered for cport id %d\n",
			cport_id);
		return -EINVAL;
	}

	for (i = 0; i < driver->count; i++) {
		if (driver->operations[id].id < id_min) {
			pr_err("Operations must sorted by operation id\n");
			return -EINVAL;
		}

		if (driver->operations[id].id == id_min) {
			pr_err("Duplicated operation id\n");
			return -EINVAL;
		}

		id_min = driver->operations[id].id;
		id++;
	}

	return g_platform->greybus_register_driver(intf_id, cport_id, driver);
}

void greybus_unregister_driver(uint8_t intf_id, uint16_t cport_id)
{
	g_platform->greybus_register_driver(intf_id, cport_id, NULL);
}


int greybus_init(struct greybus_platform *platform)
{
	TAILQ_INIT(&operations);
	if (!platform)
		return -EINVAL;
	g_platform = platform;

	return 0;
}
