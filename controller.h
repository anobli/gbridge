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
 * Copyright (c) 2016 Alexandre Bailon
 */

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <pthread.h>
#include <sys/queue.h>

#include <gbridge.h>

struct connection {
	uint16_t cport1_id;
	uint16_t cport2_id;
	struct interface *intf;

	void *priv;

	 TAILQ_ENTRY(connection) node;
	pthread_t thread;
};

struct interface {
	uint32_t vendor_id;
	uint32_t product_id;
	uint64_t serial_id;

	void *priv;

	/* gb intf private data */
	uint8_t id;
	struct controller *ctrl;
	 TAILQ_ENTRY(interface) node;
	pthread_t thread;
};

struct controller {
	const char *name;

	int (*init) (struct controller * ctrl);
	void (*exit) (struct controller * ctrl);
	int (*event_loop) (struct controller * ctrl);
	void (*event_loop_stop) (struct controller * ctrl);

	int (*interface_create) (struct interface * intf);
	void (*interface_destroy) (struct interface * intf);
	int (*connection_create) (struct connection * conn);
	int (*connection_destroy) (struct connection * conn);

	int (*write) (struct connection * conn, void *data, size_t len);
	int (*read) (struct connection * conn, void *data, size_t len);
	int (*intf_read) (struct interface * intf,
			  uint16_t * cport_id, void *data, size_t len);

	void *priv;

	/* gb controller private data */
	pthread_t thread;
	 TAILQ_ENTRY(controller) node;
	 TAILQ_HEAD(head, interface) interfaces;
};

extern struct controller bluetooth_controller;
extern struct controller tcpip_controller;

void cport_pack(struct gb_operation_msg_hdr *header, uint16_t cport_id);
uint16_t cport_unpack(struct gb_operation_msg_hdr *header);
void cport_clear(struct gb_operation_msg_hdr *header);
int read_gb_msg(int fd, void *data, size_t len);

struct interface *interface_create(struct controller *ctrl,
				   uint32_t vendor_id, uint32_t product_id,
				   uint64_t serial_id, void *priv);
int interface_hotplug(struct interface *intf);
int interface_hot_unplug(struct interface *intf);
void interface_destroy(struct interface *intf);
void interfaces_destroy(struct controller *ctrl);

int connection_create(uint8_t intf1_id, uint16_t cport1_id,
		      uint8_t intf2_id, uint16_t cport2_id);
int connection_destroy(uint8_t intf1_id, uint16_t cport1_id,
		       uint8_t intf2_id, uint16_t cport2_id);

int controller_write(uint16_t cport_id, void *data, size_t len);
void controllers_init(void);
void controllers_exit(void);

#endif				/* __CONTROLLER_H__ */
