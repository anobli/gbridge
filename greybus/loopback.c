/*
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

 * Author: Alexandre Bailon <abailon@baylibre.com>
 * Copyright (c) 2017 BayLibre
 */

#include <errno.h>
#include <string.h>

#include <greybus/libgreybus.h>

static int gb_loopback_transfer_response(struct operation *op,
					 uint32_t len, uint32_t reserved0,
					 uint32_t reserved1, uint8_t *data)
{
	struct gb_loopback_transfer_response *resp;
	size_t op_size = sizeof(*resp);

	op_size += len;
	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->len = htole32(len);
	resp->reserved0 = htole32(reserved0);
	resp->reserved1 = htole32(reserved1);
	memcpy(resp->data, data, len);

	return 0;
}

static int gb_loopback_transfer_request(struct operation *op)
{
	struct gb_loopback_transfer_request *req;
	uint32_t len;
	uint32_t reserved0;
	uint32_t reserved1;
	uint8_t *data;

	req = operation_to_request(op);
	len = le32toh(req->len);
	reserved0 = le32toh(req->reserved0);
	reserved1 = le32toh(req->reserved1);
	data = req->data;

	return gb_loopback_transfer_response(op, len, reserved0,
					     reserved1, data);
}

static struct operation_handler loopback_operations[] = {
	REQUEST_EMPTY_HANDLER(GB_REQUEST_TYPE_CPORT_SHUTDOWN),
	REQUEST_EMPTY_HANDLER(GB_LOOPBACK_TYPE_PING),
	REQUEST_HANDLER(GB_LOOPBACK_TYPE_TRANSFER, gb_loopback_transfer_request),
	REQUEST_EMPTY_HANDLER(GB_LOOPBACK_TYPE_SINK),

};

static struct greybus_driver loopback_driver = {
	.name = "loopback",
	.operations = loopback_operations,
	.count = OPERATION_COUNT(loopback_operations),
};

int loopback_register_driver(uint8_t intf_id, uint16_t cport_id) {
	return greybus_register_driver(intf_id, cport_id, &loopback_driver);
}

void loopback_unregister_driver(uint8_t intf_id, uint16_t cport_id) {
	greybus_unregister_driver(intf_id, cport_id);
}
