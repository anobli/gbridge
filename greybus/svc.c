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
	resp->status = GB_SVC_OP_SUCCESS;

	return 0;
}

static int svc_interface_resume_response(struct operation *op,
					   uint8_t intf_type)
{
	struct gb_svc_intf_resume_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = GB_SVC_OP_SUCCESS;

	return 0;
}

static int svc_interface_set_pwrm_response(struct operation *op,
					   uint8_t result_code)
{
	struct gb_svc_intf_set_pwrm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->result_code = result_code;

	return 0;
}

static int svc_pwrmon_rail_count_get_response(struct operation *op,
					      uint8_t rail_count)
{
	struct gb_svc_pwrmon_rail_count_get_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->rail_count = rail_count;

	return 0;
}

static int svc_ping_request(struct operation *op)
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

static int svc_protocol_version_response(struct operation *op)
{
       return svc_send_hello_request();
}

static int svc_interface_resume_request(struct operation *op)
{
	return svc_interface_resume_response(op, GB_SVC_INTF_TYPE_GREYBUS);
}

static int svc_interface_set_pwrm_request(struct operation *op)
{
	struct gb_svc_intf_set_pwrm_request *req;
	uint8_t tx_mode;
	uint8_t rx_mode;

	req = operation_to_request(op);
	tx_mode = req->tx_mode;
	rx_mode = req->rx_mode;

	if (tx_mode == GB_SVC_UNIPRO_HIBERNATE_MODE &&
		rx_mode == GB_SVC_UNIPRO_HIBERNATE_MODE)
		return svc_interface_set_pwrm_response(op, GB_SVC_SETPWRM_PWR_OK);
	return svc_interface_set_pwrm_response(op, GB_SVC_SETPWRM_PWR_LOCAL);
}

static int svc_pwrmon_rail_count_get_request(struct operation *op)
{
	return svc_pwrmon_rail_count_get_response(op, 0);
}

static struct operation_handler svc_operations[] = {
	REQUEST_EMPTY_HANDLER(GB_SVC_TYPE_INTF_DEVICE_ID),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_INTF_RESET),
	REQUEST_HANDLER(GB_SVC_TYPE_CONN_CREATE, svc_connection_create_request),
	REQUEST_HANDLER(GB_SVC_TYPE_CONN_DESTROY, svc_connection_destroy_request),
	REQUEST_HANDLER(GB_SVC_TYPE_DME_PEER_GET, svc_dme_peer_get_request),
	REQUEST_HANDLER(GB_SVC_TYPE_DME_PEER_SET, svc_dme_peer_set_request),
	REQUEST_EMPTY_HANDLER(GB_SVC_TYPE_ROUTE_CREATE),
	REQUEST_EMPTY_HANDLER(GB_SVC_TYPE_ROUTE_DESTROY),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_TIMESYNC_ENABLE),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_TIMESYNC_DISABLE),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_TIMESYNC_AUTHORITATIVE),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_SET_PWRM, svc_interface_set_pwrm_request),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_INTF_EJECT),
	REQUEST_HANDLER(GB_SVC_TYPE_PING, svc_ping_request),
	REQUEST_HANDLER(GB_SVC_TYPE_PWRMON_RAIL_COUNT_GET, svc_pwrmon_rail_count_get_request),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_PWRMON_RAIL_NAMES_GET),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_PWRMON_SAMPLE_GET),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_PWRMON_INTF_SAMPLE_GET),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_TIMESYNC_WAKE_PINS_ACQUIRE),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_TIMESYNC_WAKE_PINS_RELEASE),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_TIMESYNC_PING),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_MODULE_INSERTED),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_MODULE_REMOVED),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_VSYS_ENABLE, svc_interface_v_sys_enable_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_VSYS_DISABLE, svc_interface_v_sys_disable_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_REFCLK_ENABLE, svc_interface_refclk_enable_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_REFCLK_DISABLE, svc_interface_refclk_disable_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_UNIPRO_ENABLE, svc_interface_unipro_enable_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_UNIPRO_DISABLE, svc_interface_unipro_disable_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_ACTIVATE, svc_interface_activate_request),
	REQUEST_HANDLER(GB_SVC_TYPE_INTF_RESUME, svc_interface_resume_request),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_INTF_MAILBOX_EVENT),
	REQUEST_NO_HANDLER(GB_SVC_TYPE_INTF_OOPS),
	RESPONSE_HANDLER(GB_SVC_TYPE_PROTOCOL_VERSION, svc_protocol_version_response),
	RESPONSE_EMPTY_HANDLER(GB_SVC_TYPE_SVC_HELLO),
	RESPONSE_EMPTY_HANDLER(GB_SVC_TYPE_MODULE_INSERTED),
};

static struct greybus_driver svc_driver = {
	.name = "svc",
	.operations = svc_operations,
	.count = OPERATION_COUNT(svc_operations),
};

int svc_register_driver(void) {
	return greybus_register_driver(AP_INTF_ID, SVC_CPORT, &svc_driver);
}

int svc_send_protocol_version_request(void)
{
	struct gb_svc_version_request req;
	struct operation *op;

	req.major = GB_SVC_VERSION_MAJOR;
	req.minor = GB_SVC_VERSION_MINOR;

	op = greybus_alloc_operation(GB_SVC_TYPE_PROTOCOL_VERSION,
				     &req, sizeof(req));
	if (!op)
		return -ENOMEM;

	return greybus_send_request(AP_INTF_ID, SVC_CPORT, op);
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

	return greybus_send_request(AP_INTF_ID, SVC_CPORT, op);
}

int svc_send_module_inserted_event(uint8_t intf_id,
				   uint32_t vendor_id,
				   uint32_t product_id, uint64_t serial_number)
{
	struct gb_svc_module_inserted_request req;
	struct operation *op;

	req.primary_intf_id = intf_id;
	req.intf_count = 1;

	op = greybus_alloc_operation(GB_SVC_TYPE_MODULE_INSERTED,
				     &req, sizeof(req));
	if (!op)
		return -ENOMEM;

	return greybus_send_request(AP_INTF_ID, SVC_CPORT, op);
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