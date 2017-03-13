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

#include <sys/queue.h>

#ifndef _GREYBUS_H_
#define _GREYBUS_H_

struct operation {
	struct gb_operation_msg_hdr *req;
	struct gb_operation_msg_hdr *resp;
	uint8_t intf_id;
	uint16_t cport_id;
	 TAILQ_ENTRY(operation) cnode;
};

typedef int operation_handler_t(struct operation *op);
struct operation_handler {
	uint8_t id;
	operation_handler_t *callback;
	const char *name;
};

struct greybus_driver {
	const char *name;
	struct operation_handler *operations;
	uint8_t count;
};

static inline int greybus_empty_callback(struct operation *op)
{
	return 0;
}

#define REQUEST_HANDLER(operation_id, operation_handler)		\
	{								\
		.id = operation_id,					\
		.callback = operation_handler,				\
		.name = #operation_id,					\
	}

#define REQUEST_EMPTY_HANDLER(operation_id)				\
	{								\
		.id = operation_id,					\
		.callback = greybus_empty_callback,			\
		.name = #operation_id,					\
	}

#define REQUEST_NO_HANDLER(operation_id)				\
	{								\
		.id = operation_id,					\
		.callback = NULL,					\
		.name = #operation_id,					\
	}

#define RESPONSE_HANDLER(operation_id, operation_handler)		\
	{								\
		.id = OP_RESPONSE | operation_id,			\
		.callback = operation_handler,				\
		.name = #operation_id,					\
	}

#define RESPONSE_EMPTY_HANDLER(operation_id)				\
	{								\
		.id = OP_RESPONSE | operation_id,			\
		.callback = greybus_empty_callback,			\
		.name = #operation_id,					\
	}

#define OPERATION_COUNT(operations) (sizeof(operations)/sizeof(operations[0]))

#define operation_to_request(op)	\
	(void *)((op)->req + 1)

#define operation_to_response(op)	\
	(void *)((op)->resp + 1)

#define gb_operation_msg_size(hdr)	\
	le16toh(((struct gb_operation_msg_hdr *)(hdr))->size)


int greybus_init(void);
struct operation *greybus_alloc_operation(uint8_t type,
					  void *payload, size_t len);
int greybus_alloc_response(struct operation *op, size_t size);
int greybus_register_driver(uint8_t intf_id, uint16_t cport_id,
			    struct greybus_driver *driver);
void greybus_unregister_driver(uint8_t intf_id, uint16_t cport_id);
int greybus_handler(uint8_t intf_id, uint16_t cport_id,
		    struct gb_operation_msg_hdr *hdr);
int greybus_send_request(uint8_t intf_id, uint16_t cport_id,
			 struct operation *op);

#endif /* _GREYBUS_H_ */
