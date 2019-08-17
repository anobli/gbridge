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

#include <debug.h>
#include <errno.h>
#include <string.h>

#include <greybus/libgreybus.h>
#include <greybus/manifest.h>

#define CONTROL_VERSION_MAJOR 0
#define CONTROL_VERSION_MINOR 1

static uint8_t bundle_resume(uint8_t bundle_id)
{
	return GB_CONTROL_BUNDLE_PM_OK;
}

static uint8_t bundle_suspend(uint8_t bundle_id)
{
	return GB_CONTROL_BUNDLE_PM_OK;
}

static uint8_t intf_suspend_prepare(uint8_t intf_id)
{
	return GB_CONTROL_INTF_PM_OK;
}

static uint8_t intf_deactivate_prepare(uint8_t intf_id)
{
	return GB_CONTROL_INTF_PM_OK;
}

static uint8_t intf_hibernate_abort(uint8_t intf_id)
{
	return GB_CONTROL_INTF_PM_OK;
}

static int control_version_response(struct operation *op,
				    uint8_t major, uint8_t minor)
{
	struct gb_control_version_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->major = major;
	resp->minor = minor;

	return 0;
}

static int control_version_request(struct operation *op)
{
	return control_version_response(op, CONTROL_VERSION_MAJOR,
					CONTROL_VERSION_MINOR);
}

static int get_manifest_size_response(struct operation *op,
				      uint16_t manifest_size)
{
	struct gb_control_get_manifest_size_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->size = htole16(manifest_size);

	return 0;
}

static int get_manifest_size_request(struct operation *op)
{
	uint16_t size;

	size = manifest_get_size(op->intf_id);
	return get_manifest_size_response(op, size);
}

static int get_manifest_response(struct operation *op,
				 void * manifest, size_t manifest_len)
{
	struct gb_control_get_manifest_response *resp;
	size_t op_size = sizeof(*resp);

	op_size += manifest_len;
	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	memcpy(resp->data, manifest, manifest_len);

	return 0;
}

static int get_manifest_request(struct operation *op)
{
	struct manifest *manifest;

	manifest = manifest_get(op->intf_id);
	return get_manifest_response(op, manifest->blob, manifest->size);
}

static int bundle_suspend_response(struct operation *op, uint8_t status)
{
	struct gb_control_bundle_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int bundle_suspend_request(struct operation *op)
{
	struct gb_control_bundle_pm_request *req;
	uint8_t bundle_id;
	uint8_t status;

	req = operation_to_request(op);
	bundle_id = req->bundle_id;

	status = bundle_suspend(bundle_id);

	return bundle_suspend_response(op, status);
}

static int bundle_resume_response(struct operation *op, uint8_t status)
{
	struct gb_control_bundle_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int bundle_resume_request(struct operation *op)
{
	struct gb_control_bundle_pm_request *req;
	uint8_t bundle_id;
	uint8_t status;

	req = operation_to_request(op);
	bundle_id = req->bundle_id;

	status = bundle_resume(bundle_id);

	return bundle_resume_response(op, status);
}

static int bundle_deactivate_response(struct operation *op, uint8_t status)
{
	struct gb_control_bundle_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int bundle_deactivate_request(struct operation *op)
{
	struct gb_control_bundle_pm_request *req;
	uint8_t bundle_id;
	uint8_t status;

	req = operation_to_request(op);
	bundle_id = req->bundle_id;

	status = bundle_deactivate(op->intf_id, bundle_id);

	return bundle_deactivate_response(op, status);
}

static int bundle_activate_response(struct operation *op, uint8_t status)
{
	struct gb_control_bundle_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int bundle_activate_request(struct operation *op)
{
	struct gb_control_bundle_pm_request *req;
	uint8_t bundle_id;
	uint8_t status;

	req = operation_to_request(op);
	bundle_id = req->bundle_id;

	status = bundle_activate(op->intf_id, bundle_id);

	return bundle_activate_response(op, status);
}

static int intf_suspend_prepare_response(struct operation *op, uint8_t status)
{
	struct gb_control_intf_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int intf_suspend_prepare_request(struct operation *op)
{
	uint8_t status;

	status = intf_suspend_prepare(op->intf_id);
	return intf_suspend_prepare_response(op, status);
}

static int intf_deactivate_prepare_response(struct operation *op,
					    uint8_t status)
{
	struct gb_control_intf_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int intf_deactivate_prepare_request(struct operation *op)
{
	uint8_t status;

	status = intf_deactivate_prepare(op->intf_id);
	return intf_deactivate_prepare_response(op, status);
}

static int intf_hibernate_abort_response(struct operation *op,
					    uint8_t status)
{
	struct gb_control_intf_pm_response *resp;
	size_t op_size = sizeof(*resp);

	if (greybus_alloc_response(op, op_size))
		return -ENOMEM;

	resp = operation_to_response(op);
	resp->status = status;

	return 0;
}

static int intf_hibernate_abort_request(struct operation *op)
{
	uint8_t status;

	status = intf_hibernate_abort(op->intf_id);
	return intf_hibernate_abort_response(op, status);
}

static struct operation_handler control_operations[] = {
	REQUEST_EMPTY_HANDLER(GB_REQUEST_TYPE_CPORT_SHUTDOWN),
	REQUEST_HANDLER(GB_CONTROL_TYPE_VERSION, control_version_request),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_PROBE_AP),
	REQUEST_HANDLER(GB_CONTROL_TYPE_GET_MANIFEST_SIZE, get_manifest_size_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_GET_MANIFEST, get_manifest_request),
	REQUEST_EMPTY_HANDLER(GB_CONTROL_TYPE_CONNECTED),
	REQUEST_EMPTY_HANDLER(GB_CONTROL_TYPE_DISCONNECTED),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_TIMESYNC_ENABLE),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_TIMESYNC_DISABLE),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_TIMESYNC_AUTHORITATIVE),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_BUNDLE_VERSION),
	REQUEST_EMPTY_HANDLER(GB_CONTROL_TYPE_DISCONNECTING),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_TIMESYNC_GET_LAST_EVENT),
	REQUEST_NO_HANDLER(GB_CONTROL_TYPE_MODE_SWITCH),
	REQUEST_HANDLER(GB_CONTROL_TYPE_BUNDLE_SUSPEND, bundle_suspend_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_BUNDLE_RESUME, bundle_resume_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_BUNDLE_DEACTIVATE, bundle_deactivate_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_BUNDLE_ACTIVATE, bundle_activate_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_INTF_SUSPEND_PREPARE, intf_suspend_prepare_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_INTF_DEACTIVATE_PREPARE, intf_deactivate_prepare_request),
	REQUEST_HANDLER(GB_CONTROL_TYPE_INTF_HIBERNATE_ABORT, intf_hibernate_abort_request),
};

static struct greybus_driver control_driver = {
	.name = "control",
	.operations = control_operations,
	.count = OPERATION_COUNT(control_operations),
};

int control_register_driver(uint8_t intf_id) {
	return greybus_register_driver(intf_id, CONTROL_CPORT, &control_driver);
}

void control_unregister_driver(uint8_t intf_id) {
	greybus_unregister_driver(intf_id, CONTROL_CPORT);
}