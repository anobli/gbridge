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

#include <config.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/queue.h>

#define __packed  __attribute__((__packed__))

#include <greybus.h>
#include <greybus_protocols.h>
#include <gb_netlink.h>

#define SVC_CPORT		0
#define AP_INTF_ID		0
#define OP_RESPONSE		0x80

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
        for ((var) = TAILQ_FIRST((head));                               \
            (var) && ((tvar) = TAILQ_NEXT((var), field), 1);            \
            (var) = (tvar))
#endif

int svc_init(void);
int svc_register_driver();
int svc_send_module_inserted_event(uint8_t intf_id,
				   uint32_t vendor_id,
				   uint32_t product_id, uint64_t serial_number);
void svc_watchdog_disable(void);

#endif /* _GBRIDGE_H_ */
