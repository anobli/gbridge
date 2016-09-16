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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

enum log_level {
	LL_ERROR = 0,
	LL_WARNING,
	LL_INFO,
	LL_DEBUG,
	LL_VERBOSE
};

#define ll_print(ll, format, ...)			\
	do {						\
		if (log_level >= ll)			\
			printf(format, ##__VA_ARGS__);	\
	} while (0)

#define pr_err(format, ...) \
	ll_print(LL_ERROR, format, ##__VA_ARGS__)

#define pr_warn(format, ...) \
	ll_print(LL_WARNING, format, ##__VA_ARGS__)

#define pr_info(format, ...) \
	ll_print(LL_INFO, format, ##__VA_ARGS__)

#define pr_dbg(format, ...) \
	ll_print(LL_DEBUG, format, ##__VA_ARGS__)

extern int log_level;

void set_log_level(int ll);

#endif /* _DEBUG_H_ */
