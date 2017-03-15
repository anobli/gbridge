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

#ifndef _PROTOCOLS_H_
#define _PROTOCOLS_H_

#include <stdint.h>

int control_register_driver(uint8_t intf_id);
void control_unregister_driver(uint8_t intf_id);

int loopback_register_driver(uint8_t intf_id, uint16_t cport_id);
void loopback_unregister_driver(uint8_t intf_id, uint16_t cport_id);

#endif
