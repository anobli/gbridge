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

#include <stdint.h>
#include <sys/queue.h>

struct cport {
	uint16_t id;
	uint8_t protocol_id;
	LIST_ENTRY(cport) cport_node;
};

struct bundle {
	uint8_t id;
	uint8_t class;
	LIST_ENTRY(bundle) bundle_node;
	LIST_HEAD(cport_head, cport) cports;
};

struct manifest {
	uint16_t size;
	uint8_t intf_id;
	LIST_ENTRY(manifest) manifest_node;
	LIST_HEAD(bundle_head, bundle) bundles;

	void *blob;
};

struct manifest *parse_manifest(void *manifest_blob, uint8_t intf_id);
void manifest_free(struct manifest *manifest);
struct manifest *manifest_get(uint8_t intf_id);
uint16_t manifest_get_size(uint8_t intf_id);
uint8_t bundle_activate(uint8_t intf_id, uint8_t bundle_id);
uint8_t bundle_deactivate(uint8_t intf_id, uint8_t bundle_id);