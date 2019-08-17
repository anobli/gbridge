/*
 * GBridge (Greybus Bridge)
 * Copyright (c) 2017 Alexandre Bailon
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
#include <stdio.h>
#include <string.h>

#include <debug.h>
#include <controller.h>
#include <greybus/protocols.h>
#include <greybus/manifest.h>

struct gbsim_controller {
	const char *manifest_file;
	struct manifest *manifest;
};

static int gbsim_init(struct controller *ctrl)
{
	/* TODO: check if the manifest is valid here */
	return 0;
}

static void gbsim_exit(struct controller * ctrl)
{

}

static int manifest_load(struct gbsim_controller *gbsim_ctrl,
			 struct interface *intf)
{
	FILE *f;
	int ret = 0;
	uint16_t size, le_size;
	void *manifest_blob;

	pr_dbg("Loading the manifest %s\n", gbsim_ctrl->manifest_file);
	f = fopen(gbsim_ctrl->manifest_file, "r");
	if (!f) {
		perror("Failed to open manifest file");
		return errno;
	}

	/* Get manifest size to allocate a buffer to store it */
	if (fread(&le_size, sizeof(le_size), 1, f) != 1) {
		pr_err("Failed to read manifest size\n");
		ret = -EINVAL;
		goto err_close_manifest;
	}
	size = le16toh(le_size);

	manifest_blob = malloc(size);
	if (!manifest_blob) {
		ret = -ENOMEM;
		goto err_close_manifest;
	}

	fseek(f, 0, SEEK_SET);
	if (fread(manifest_blob, 1, size, f) != size) {
		pr_err("Failed to read manifest\n");
		ret = -EINVAL;
		goto err_free_manifest;
	}

	gbsim_ctrl->manifest = parse_manifest(manifest_blob, intf->id);
	if (!gbsim_ctrl->manifest) {
		pr_err("Failed to parse the manifest");
		ret = -EINVAL;
		goto err_free_manifest;
	}

	pr_dbg("Manifest loaded\n");

err_free_manifest:
	free(manifest_blob);
err_close_manifest:
	fclose(f);

	return ret;
}

static int gbsim_hotplug(struct controller * ctrl)
{
	int ret = 0;
	struct interface *intf;
	struct gbsim_controller *gbsim_ctrl = ctrl->priv;

	/* FIXME: use real IDs */
	intf = interface_create(ctrl, 1, 1, 0x1234, NULL);
	if (!intf) {
		pr_err("Failed to create GBSIM interface\n");
		return -ENOMEM;
	}

	ret = manifest_load(gbsim_ctrl, intf);
	if (ret < 0)
		goto err_interface_destroy;

	ret = control_register_driver(intf->id);
	if (ret < 0)
		goto err_manifest_free;

	ret = interface_hotplug(intf);
	if (ret < 0)
		goto err_unregister_driver;

	return 0;

err_unregister_driver:
	control_unregister_driver(intf->id);
err_manifest_free:
	manifest_free(gbsim_ctrl->manifest);
err_interface_destroy:
	interface_destroy(intf);

	return ret;
}

static int gbsim_write(struct connection * conn, void *data, size_t len)
{
	int ret;

	ret = greybus_handler(conn->intf2->id, conn->cport2_id, data);
	if (ret)
		return ret;

	return 0;
}

struct controller gbsim_controller = {
	.name = "gbsim",
	.init = gbsim_init,
	.exit = gbsim_exit,
	.write = gbsim_write,
	.event_loop = gbsim_hotplug,
};

int register_gbsim_controller(const char *manifest_file)
{
	struct controller *ctrl;
	struct gbsim_controller *gbsim_ctrl;

	gbsim_ctrl = malloc(sizeof(*gbsim_ctrl));
	if (!gbsim_ctrl)
		return -ENOMEM;

	gbsim_ctrl->manifest_file = manifest_file;

	ctrl = malloc(sizeof(*ctrl));
	if (!ctrl) {
		free(gbsim_ctrl);
		return -ENOMEM;
	}

	memcpy(ctrl, &gbsim_controller, sizeof(*ctrl));
	ctrl->priv = gbsim_ctrl;
	register_controller(ctrl);

	return 0;
}
