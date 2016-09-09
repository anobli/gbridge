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

#include <debug.h>

int log_level = LL_VERBOSE;

void set_log_level(int ll)
{
	log_level = ll;
}

void _pr_dump(const char *fn, uint8_t *data, size_t len)
{
	int j;
	int i = 0;

	if (log_level < LL_VERBOSE)
		return;

	printf("%s:\n", fn);
	while (i < len) {
		for (j = 0; j < LINE_COUNT; i++, j++) {
			if (i >= len)
				break;
			printf("%02x ", data[i]);
		}
		printf("\n");
	}
}
