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
#include <endian.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <debug.h>
#include <gbridge.h>
#include <controller.h>

/* TODO: Can we use other IDs ? */
#define ENDO_ID 0x4755
#define AP_INTF_ID 0x0

static int svc_send_hello_request(void);

static int svc_dme_peer_get_response(struct operation *op, uint16_t result_code,
				     uint32_t attr_value)
{
	struct gb_svc_dme_peer_get_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = htole16(result_code);
	resp->attr_value = htole32(attr_value);

	return 0;
}

static int svc_dme_peer_set_response(struct operation *op, uint16_t result_code)
{
	struct gb_svc_dme_peer_set_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = htole16(result_code);

	return 0;
}

static int svc_interface_v_sys_enable_response(struct operation *op,
					       uint8_t result_code)
{
	struct gb_svc_intf_vsys_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_interface_v_sys_disable_response(struct operation *op,
						uint8_t result_code)
{
	struct gb_svc_intf_vsys_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_interface_refclk_enable_response(struct operation *op,
						uint8_t result_code)
{
	struct gb_svc_intf_refclk_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_interface_refclk_disable_response(struct operation *op,
						 uint8_t result_code)
{
	struct gb_svc_intf_refclk_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_interface_unipro_enable_response(struct operation *op,
						uint8_t result_code)
{
	struct gb_svc_intf_unipro_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_interface_unipro_disable_response(struct operation *op,
						 uint8_t result_code)
{
	struct gb_svc_intf_unipro_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_interface_activate_response(struct operation *op,
					   uint8_t intf_type)
{
	struct gb_svc_intf_activate_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->intf_type = intf_type;

	return 0;
}

static int svc_ping_request(struct operation *op)
{
	return 0;
}

static int svc_interface_device_id_request(struct operation *op)
{
	return 0;
}

static int svc_connection_create_request(struct operation *op)
{
	struct gb_svc_conn_create_request *req;
	uint8_t intf1_id;
	uint16_t cport1_id;
	uint8_t intf2_id;
	uint16_t cport2_id;

	req = operation_to_request(op);
	intf1_id = req->intf1_id;
	cport1_id = le16toh(req->cport1_id);
	intf2_id = req->intf2_id;
	cport2_id = le16toh(req->cport2_id);

	return connection_create(intf1_id, cport1_id, intf2_id, cport2_id);
}

static int svc_connection_destroy_request(struct operation *op)
{
	struct gb_svc_conn_destroy_request *req;
	uint8_t intf1_id;
	uint16_t cport1_id;
	uint8_t intf2_id;
	uint16_t cport2_id;

	req = operation_to_request(op);
	intf1_id = req->intf1_id;
	cport1_id = le16toh(req->cport1_id);
	intf2_id = req->intf2_id;
	cport2_id = le16toh(req->cport2_id);

	return connection_destroy(intf1_id, cport1_id, intf2_id, cport2_id);
}

static int svc_dme_peer_get_request(struct operation *op)
{
	return svc_dme_peer_get_response(op, 0, 0x0126);
}

static int svc_dme_peer_set_request(struct operation *op)
{
	return svc_dme_peer_set_response(op, 0);
}

static int svc_interface_v_sys_enable_request(struct operation *op)
{
	return svc_interface_v_sys_enable_response(op, GB_SVC_INTF_VSYS_OK);
}

static int svc_interface_v_sys_disable_request(struct operation *op)
{
	return svc_interface_v_sys_disable_response(op, GB_SVC_INTF_VSYS_OK);;
}

static int svc_interface_refclk_enable_request(struct operation *op)
{
	return svc_interface_refclk_enable_response(op, GB_SVC_INTF_REFCLK_OK);
}

static int svc_interface_refclk_disable_request(struct operation *op)
{
	return svc_interface_refclk_disable_response(op, GB_SVC_INTF_REFCLK_OK);
}

static int svc_interface_unipro_enable_request(struct operation *op)
{
	return svc_interface_unipro_enable_response(op, GB_SVC_INTF_UNIPRO_OK);
}

static int svc_interface_unipro_disable_request(struct operation *op)
{
	return svc_interface_unipro_disable_response(op, GB_SVC_INTF_UNIPRO_OK);
}

static int svc_interface_activate_request(struct operation *op)
{
	return svc_interface_activate_response(op, GB_SVC_INTF_TYPE_GREYBUS);
}

static int svc_interface_hotplug_request(struct operation *op)
{
	return -ENOSYS;
}

static int svc_interface_hot_unplug_request(struct operation *op)
{
	return -ENOSYS;
}

static int svc_handler_response(struct operation *op)
{
	switch (op->resp->type & ~OP_RESPONSE) {
	case GB_REQUEST_TYPE_PROTOCOL_VERSION:
		return svc_send_hello_request();
	case GB_SVC_TYPE_SVC_HELLO:
	case GB_SVC_TYPE_INTF_HOTPLUG:
		return 0;
	}

	return -EINVAL;
}

int svc_handler(struct operation *op)
{
	if (op->resp)
		return svc_handler_response(op);

	switch (op->req->type) {
	case GB_SVC_TYPE_PING:
		return svc_ping_request(op);
	case GB_SVC_TYPE_INTF_DEVICE_ID:
		return svc_interface_device_id_request(op);
	case GB_SVC_TYPE_CONN_CREATE:
		return svc_connection_create_request(op);
	case GB_SVC_TYPE_CONN_DESTROY:
		return svc_connection_destroy_request(op);
	case GB_SVC_TYPE_DME_PEER_GET:
		return svc_dme_peer_get_request(op);
	case GB_SVC_TYPE_DME_PEER_SET:
		return svc_dme_peer_set_request(op);
	case GB_SVC_TYPE_INTF_HOTPLUG:
		return svc_interface_hotplug_request(op);
	case GB_SVC_TYPE_INTF_HOT_UNPLUG:
		return svc_interface_hot_unplug_request(op);
	case GB_SVC_TYPE_INTF_VSYS_ENABLE:
		return svc_interface_v_sys_enable_request(op);
	case GB_SVC_TYPE_INTF_VSYS_DISABLE:
		return svc_interface_v_sys_disable_request(op);
	case GB_SVC_TYPE_INTF_REFCLK_ENABLE:
		return svc_interface_refclk_enable_request(op);
	case GB_SVC_TYPE_INTF_REFCLK_DISABLE:
		return svc_interface_refclk_disable_request(op);
	case GB_SVC_TYPE_INTF_UNIPRO_ENABLE:
		return svc_interface_unipro_enable_request(op);
	case GB_SVC_TYPE_INTF_UNIPRO_DISABLE:
		return svc_interface_unipro_disable_request(op);
	case GB_SVC_TYPE_INTF_ACTIVATE:
		return svc_interface_activate_request(op);
	}

	return -EINVAL;
}

int svc_send_protocol_version_request(void)
{
	struct gb_protocol_version_request req;
	struct operation *op;

	req.major = GB_SVC_VERSION_MAJOR;
	req.minor = GB_SVC_VERSION_MINOR;

	op = greybus_alloc_operation(GB_REQUEST_TYPE_PROTOCOL_VERSION,
				     &req, sizeof(req));
	if (!op)
		return -ENOMEM;

	return greybus_send_request(0, op);
}

int svc_send_hello_request(void)
{
	struct gb_svc_hello_request req;
	struct operation *op;

	req.endo_id = htole16(ENDO_ID);
	req.interface_id = AP_INTF_ID;

	op = greybus_alloc_operation(GB_SVC_TYPE_SVC_HELLO, &req, sizeof(req));
	if (!op)
		return -ENOMEM;

	return greybus_send_request(0, op);
}

int svc_send_intf_hotplug_event(uint8_t intf_id,
				uint32_t vendor_id,
				uint32_t product_id, uint64_t serial_number)
{
	struct gb_svc_intf_hotplug_request req;
	struct operation *op;

	req.intf_id = intf_id;
	req.data.ara_vend_id = htole32(vendor_id);
	req.data.ara_prod_id = htole32(product_id);
	req.data.serial_number = htole64(serial_number);

	//FIXME: Use some real version numbers here ?
	req.data.ddbl1_mfr_id = htole32(1);
	req.data.ddbl1_prod_id = htole32(1);

	op = greybus_alloc_operation(GB_SVC_TYPE_INTF_HOTPLUG,
				     &req, sizeof(req));
	if (!op)
		return -ENOMEM;

	return greybus_send_request(0, op);
}

int svc_init(void)
{
	return svc_send_protocol_version_request();
}

void svc_watchdog_disable(void)
{
	int fd;

	/* FIXME: Shouldn't be hardcoded */
	fd = open("/sys/bus/greybus/devices/1-svc/watchdog", O_WRONLY);
	if (fd < 0) {
		perror("Failed to open watchdog");
		return;
	}

	if (write(fd, "0", 1) != 1)
		perror("Failed to disable watchdog\n");
}