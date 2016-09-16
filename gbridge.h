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

#ifndef _GBRIDGE_H_
#define _GBRIDGE_H_

#include <stdint.h>
#include <linux/types.h>
#include <sys/queue.h>

#define __packed  __attribute__((__packed__))

/* Include kernel headers */
#include <greybus.h>
#include <greybus_protocols.h>
#include <gb_netlink.h>

#define SVC_CPORT		0

#define gb_operation_msg_size(hdr)	\
	le16toh(((struct gb_operation_msg_hdr *)(hdr))->size)

#endif /* _GBRIDGE_H_ */
