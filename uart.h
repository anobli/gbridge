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

#ifndef _UART_H_
#define _UART_H_

#include <config.h>
#include <debug.h>

#ifdef HAVE_UART
int register_uart_controller(const char *file_name, int baudrate);
#else
int register_uart_controller(const char *file_name, int baudrate)
{
	pr_err("UART support has not been compiled.\n");

	return -1;
}
#endif

#endif /* _UART_H_ */