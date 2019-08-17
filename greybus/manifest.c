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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <debug.h>
#include <greybus/libgreybus.h>
#include <greybus/protocols.h>
#include <greybus/manifest.h>

LIST_HEAD(manifest_head, manifest) manifests = LIST_HEAD_INITIALIZER(manifests);

#ifndef LIST_FOREACH_SAFE
#define	LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = LIST_FIRST((head));				\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))
#endif

static struct bundle *find_bundle(struct manifest *manifest, uint8_t id)
{
	struct bundle *bundle;

	LIST_FOREACH(bundle, &manifest->bundles, bundle_node) {
		if (bundle->id == id)
			return bundle;
	}

	return NULL;
}

static struct bundle *bundle_alloc(struct manifest *manifest, uint8_t id)
{
	struct bundle *bundle;

	bundle = malloc(sizeof(*bundle));
	if (!bundle)
		return NULL;

	bundle->id = id;
	LIST_INIT(&bundle->cports);
	LIST_INSERT_HEAD(&manifest->bundles, bundle, bundle_node);

	return bundle;
}

static struct bundle *find_bundle_alloc(struct manifest *manifest, uint8_t id)
{
	struct bundle *bundle;

	bundle = find_bundle(manifest, id);
	if (!bundle)
		bundle = bundle_alloc(manifest, id);

	return bundle;
}

static int parse_descriptor_cport(struct manifest *manifest,
				  struct greybus_descriptor_cport *desc)
{
	struct cport *cport;
	struct bundle *bundle;

	bundle = find_bundle_alloc(manifest, desc->bundle);
	if (!bundle)
		return -ENOMEM;

	cport = malloc(sizeof(*cport));
	if (!cport)
		return -ENOMEM;

	cport->id = le16toh(desc->id);
	cport->protocol_id = desc->protocol_id;
	LIST_INSERT_HEAD(&bundle->cports, cport, cport_node);

	pr_dbg("cport_id = %u, protocol_id = %u\n",
		cport->id, cport->protocol_id);

	return 0;
}

static int parse_descriptor_bundle(struct manifest *manifest,
				   struct greybus_descriptor_bundle *desc)
{
	struct bundle *bundle;

	bundle = find_bundle_alloc(manifest, desc->id);
	if (!bundle)
		return -ENOMEM;
	bundle->class = desc->class;

	pr_dbg("bundle_id = %u, class = %u\n", bundle->id, bundle->class);

	return 0;
}

static int parse_descriptor(struct manifest *manifest,
			    struct greybus_descriptor *desc)
{
	int ret;
	uint16_t size;

	size = le16toh(desc->header.size);
	pr_dbg("Parsing a descriptor\nSize: %u\n", size);
	switch (desc->header.type) {
		case GREYBUS_TYPE_INTERFACE:
			pr_dbg("Type: interface descriptor\n");
			break;
		case GREYBUS_TYPE_STRING:
			pr_dbg("Type: string descriptor\n");
			break;
		case GREYBUS_TYPE_BUNDLE:
			pr_dbg("Type: bundle descriptor\n");
			ret = parse_descriptor_bundle(manifest, &desc->bundle);
			break;
		case GREYBUS_TYPE_CPORT:
			pr_dbg("Type: cport descriptor\n");
			ret = parse_descriptor_cport(manifest, &desc->cport);
			if (ret < 0)
				return ret;
			break;
		default:
			pr_err("Unknown descriptor type\n");
	}

	return size;
}

void manifest_free(struct manifest *manifest)
{
	struct cport *cport, *tmp_cport;
	struct bundle *bundle, *tmp_bundle;

	LIST_FOREACH_SAFE(bundle, &manifest->bundles, bundle_node, tmp_bundle) {
		LIST_FOREACH_SAFE(cport, &bundle->cports, cport_node, tmp_cport) {
			LIST_REMOVE(cport, cport_node);
			free(cport);
		}
		LIST_REMOVE(bundle, bundle_node);
		free(bundle);
	}
	LIST_REMOVE(manifest, manifest_node);
	free(manifest->blob);
	free(manifest);
}

struct manifest *parse_manifest(void *manifest_blob, uint8_t intf_id)
{
	int ret;
	uint16_t size;
	uint8_t *p_desc;
	struct manifest *manifest;
	struct greybus_manifest *greybus_manifest;

	pr_dbg("Parsing the manifest for interface %u\n", intf_id);

	manifest = malloc(sizeof(*manifest));
	if (!manifest)
		return NULL;
	manifest->blob = NULL;

	greybus_manifest = manifest_blob;
	p_desc = (uint8_t *)greybus_manifest->descriptors;
	manifest->size = le16toh(greybus_manifest->header.size);
	pr_dbg("Manifest size: %u\n", manifest->size);

	size = sizeof(struct greybus_manifest_header);
	do {
		ret = parse_descriptor(manifest,
				       (struct greybus_descriptor *)p_desc);
		if (ret <= 0)
			goto err_manifest_free;
		size += (uint16_t)ret;
		p_desc += (uint16_t)ret;
	} while(size < manifest->size);

	if (size != manifest->size) {
		ret = -EINVAL;
		goto err_manifest_free;
	}

	manifest->blob = malloc(manifest->size);
	if (!manifest->blob)
		goto err_manifest_free;
	memcpy(manifest->blob, manifest_blob, manifest->size);
	manifest->intf_id = intf_id;

	LIST_INSERT_HEAD(&manifests, manifest, manifest_node);

	return manifest;

err_manifest_free:
	manifest_free(manifest);

	return NULL;
}

struct manifest *manifest_get(uint8_t intf_id)
{
	struct manifest *manifest;

	LIST_FOREACH(manifest, &manifests, manifest_node) {
		if (manifest->intf_id == intf_id)
			return manifest;
	}

	return NULL;
}

uint16_t manifest_get_size(uint8_t intf_id)
{
	struct manifest *manifest = manifest_get(intf_id);

	return !manifest ? 0 : manifest->size;
}

static int cport_enable(uint8_t intf_id, struct cport *cport)
{
	int ret = -EINVAL;

	switch (cport->protocol_id) {
		case GREYBUS_PROTOCOL_LOOPBACK:
			ret = loopback_register_driver(intf_id, cport->id);
			break;
		default:
			pr_err("Unsupported protocol\n");
	}

	return ret;
}

static int cport_disable(uint8_t intf_id, struct cport *cport)
{
	int ret = 0;

	switch (cport->protocol_id) {
		case GREYBUS_PROTOCOL_LOOPBACK:
			loopback_unregister_driver(intf_id, cport->id);
			break;
		default:
			pr_err("Unsupported protocol\n");
			ret = -EINVAL;
	}

	return ret;
}

static uint8_t _bundle_activate(uint8_t intf_id, uint8_t bundle_id,
				uint8_t activate)
{
	struct manifest *manifest;
	struct bundle *bundle;
	struct cport *cport;
	int ret;

	manifest = manifest_get(intf_id);
	if (!manifest) {
		pr_err("Failed to get the manifest for interface %u\n",
			intf_id);
		return GB_CONTROL_BUNDLE_PM_INVAL;
	}

	bundle = find_bundle(manifest, bundle_id);
	if (!bundle) {
		pr_err("Failed to get the bundle %u for interface %u\n",
			bundle_id, intf_id);
		return GB_CONTROL_BUNDLE_PM_INVAL;
	}

	LIST_FOREACH(cport, &bundle->cports, cport_node) {
		if (activate)
			ret = cport_enable(intf_id, cport);
		else
			ret = cport_disable(intf_id, cport);
		if (ret) {
			pr_err("Failed to %s the cport %u for interface %u\n",
				activate ? "activate" : "deactivate",
				cport->id, intf_id);
			return GB_CONTROL_BUNDLE_PM_INVAL;
		}
	}

	return GB_CONTROL_BUNDLE_PM_OK;
}

uint8_t bundle_activate(uint8_t intf_id, uint8_t bundle_id)
{
	return _bundle_activate(intf_id, bundle_id, 1);
}

uint8_t bundle_deactivate(uint8_t intf_id, uint8_t bundle_id)
{
	return _bundle_activate(intf_id, bundle_id, 0);
}
