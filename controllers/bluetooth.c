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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include <debug.h>
#include <gbridge.h>
#include <controller.h>

#define BDADDR_SIZE	19
#define BDNAME_SIZE	248

struct bluetooth_device {
	char name[BDNAME_SIZE];
	char addr[BDADDR_SIZE];
	struct btd_device *device;
	int sock;
};

struct bluetooth_controller {
	int dev_id;
	int sock;
};

static int bluetooth_is_connected(struct controller *ctrl, bdaddr_t *bdaddr)
{
	char addr[BDADDR_SIZE];
	struct interface *intf;
	struct bluetooth_device *bd;

	ba2str(bdaddr, addr);
	TAILQ_FOREACH(intf, &ctrl->interfaces, node) {
		bd = intf->priv;
		if (strcmp(addr, bd->addr) == 0) {
			return 1;
		}
	}

	return 0;
}

static int bluetooth_connect(struct controller *ctrl, bdaddr_t *bdaddr)
{
	int ret;
	struct bluetooth_device *bd;
	struct interface *intf;
	struct sockaddr_rc addr;
	struct bluetooth_controller *bt_ctrl = ctrl->priv;

	bd = malloc(sizeof(*bd));
	if (!bd)
		return -ENOMEM;

	ba2str(bdaddr, bd->addr);
	memset(bd->name, 0, sizeof(bd->name));
	ret = hci_read_remote_name(bt_ctrl->sock, bdaddr,
				   sizeof(bd->name), bd->name,
				   IREQ_CACHE_FLUSH);
	if (ret < 0)
		goto err_free_bd;

	pr_info("Connecting a new Greybus device\n");

	bd->sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (bd->sock < 0) {
		ret = bd->sock;
		goto err_free_bd;
	}

	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;
	str2ba(bd->addr, &addr.rc_bdaddr);

	ret = connect(bd->sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0)
		goto err_close_sock;

	pr_info("Greybus device connected\n");

	/* FIXME: use real IDs */
	intf = interface_create(ctrl, 1, 1, 0x1234, bd);
	if (!intf)
		goto err_close_sock;

	ret = interface_hotplug(intf);
	if (ret < 0)
		goto err_intf_destroy;

	return 0;

 err_intf_destroy:
	interface_destroy(intf);
 err_close_sock:
	close(bd->sock);
 err_free_bd:
	free(bd);

	return ret;
}

static void bluetooth_disconnect(struct controller *ctrl,
				 struct bluetooth_device *bd)
{
	close(bd->sock);
	free(bd);
}

static void bluetooth_interface_destroy(struct interface *intf)
{
	bluetooth_disconnect(intf->ctrl, intf->priv);
}

static int bluetooth_scan(struct controller *ctrl)
{
	int ret;
	inquiry_info *ii = NULL;
	int max_rsp, num_rsp;
	int len, flags;
	int i;
	char name[BDNAME_SIZE] = { 0 };
	struct bluetooth_controller *bt_ctrl = ctrl->priv;

	len = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;

	ii = malloc(max_rsp * sizeof(inquiry_info));
	if (!ii)
		return -ENOMEM;

	while (1) {
		num_rsp = hci_inquiry(bt_ctrl->dev_id,
				      len, max_rsp, NULL, &ii, flags);
		if (num_rsp < 0) {
			perror("hci_inquiry");
			free(ii);
			return errno;
		}

		for (i = 0; i < num_rsp; i++) {
			memset(name, 0, sizeof(name));
			ret =
			    hci_read_remote_name(bt_ctrl->sock,
						 &(ii + i)->bdaddr,
						 sizeof(name), name, 0);
			if (ret < 0)
				strcpy(name, "[unknown]");

			if (strstr(name, "GREYBUS")) {
				if (!bluetooth_is_connected
				    (ctrl, &(ii + i)->bdaddr)) {
					bluetooth_connect(ctrl,
							  &(ii + i)->bdaddr);
				}
			}
		}
	}

	free(ii);

	return 0;
}

static int bluetooth_write(struct connection *conn, void *data, size_t len)
{
	struct interface *intf = conn->intf;
	struct bluetooth_device *bd = intf->priv;

	cport_pack(data, conn->cport2_id);
	return write(bd->sock, data, len);
}

static int bluetooth_read(struct interface *intf,
			  uint16_t *cport_id, void *data, size_t len)
{
	int ret;
	struct bluetooth_device *bd = intf->priv;

	ret = read_gb_msg(bd->sock, data, len);
	if (ret > 0)
		*cport_id = cport_unpack(data);
	return ret;
}

static int bluetooth_init(struct controller *ctrl)
{
	int ret;
	struct bluetooth_controller *bt_ctrl;

	bt_ctrl = malloc(sizeof(*bt_ctrl));
	if (!bt_ctrl)
		return -ENOMEM;
	 ctrl->priv = bt_ctrl;

	bt_ctrl->dev_id = hci_get_route(NULL);
	if (bt_ctrl->dev_id < 0) {
		perror("Failed to get device id");
		ret = errno;
		goto err_free_bt_ctrl;
	}

	bt_ctrl->sock = hci_open_dev(bt_ctrl->dev_id);
	if (bt_ctrl->sock < 0) {
		perror("Failed to open socket");
		ret = errno;
		goto err_free_bt_ctrl;
	}

	return 0;

err_free_bt_ctrl:
	free(bt_ctrl);

	return ret;
}

static void bluetooth_exit(struct controller *ctrl)
{
	struct bluetooth_controller *bt_ctrl = ctrl->priv;

	free(ctrl->priv);
	close(bt_ctrl->sock);
}

struct controller bluetooth_controller = {
	.name = "bluetooth",
	.init = bluetooth_init,
	.exit = bluetooth_exit,
	.event_loop = bluetooth_scan,
	.write = bluetooth_write,
	.intf_read = bluetooth_read,
	.interface_destroy = bluetooth_interface_destroy,
};
