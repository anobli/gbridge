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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <debug.h>
#include <gbridge.h>
#include <controller.h>

struct tcpip_connection {
	int sock;
};

struct tcpip_device {
	char *host_name;
	char addr[AVAHI_ADDRESS_STR_MAX];
	int port;
};

struct tcpip_controller {
	AvahiClient *client;
	AvahiSimplePoll *simple_poll;
};

static int tcpip_connection_create(struct connection *conn)
{
	int ret;
	struct sockaddr_in serv_addr;
	struct tcpip_connection *tconn;
	struct tcpip_device *td = conn->intf->priv;

	tconn = malloc(sizeof(*tconn));
	if (!tconn)
		return -ENOMEM;

	tconn->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (tconn->sock < 0) {
		pr_err("Can't create socket\n");
		return tconn->sock;
	}
	conn->priv = tconn;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(td->port + conn->cport2_id);
	serv_addr.sin_addr.s_addr = inet_addr(td->addr);

	pr_info("Trying to connect to module at %s:%d\n", td->addr, td->port);
	do {
		ret = connect(tconn->sock,
			      (struct sockaddr *)&serv_addr,
			      sizeof(struct sockaddr));
		if (ret)
			sleep(1);
	} while (ret);
	pr_info("Connected to module\n");

	return 0;
}

static int tcpip_connection_destroy(struct connection *conn)
{
	struct tcpip_connection *tconn = conn->priv;

	conn->priv = NULL;
	close(tconn->sock);
	free(tconn);

	return 0;
}

static void tcpip_hotplug(struct controller *ctrl, const char *host_name,
			  const AvahiAddress *address, uint16_t port)
{
	struct interface *intf;
	struct tcpip_device *td;

	td = malloc(sizeof(*td));
	if (!td)
		goto exit;

	td->port = port;
	avahi_address_snprint(td->addr, sizeof(td->addr), address);
	td->host_name = malloc(strlen(host_name) + 1);
	if (!td->host_name)
		goto err_free_td;
	strcpy(td->host_name, host_name);

	/* FIXME: use real IDs */
	intf = interface_create(ctrl, 1, 1, 0x1234, td);
	if (!intf)
		goto err_free_host_name;

	if (interface_hotplug(intf))
		goto err_intf_destroy;

	return;

err_intf_destroy:
	interface_destroy(intf);
err_free_host_name:
	free(td->host_name);
err_free_td:
	free(td);
exit:
	pr_err("Failed to hotplug of TCP/IP module\n");
}

static void resolve_callback(AvahiServiceResolver *r,
			     AvahiIfIndex interface,
			     AvahiProtocol protocol,
			     AvahiResolverEvent event,
			     const char *name,
			     const char *type,
			     const char *domain,
			     const char *host_name,
			     const AvahiAddress *address,
			     uint16_t port,
			     AvahiStringList *txt,
			     AvahiLookupResultFlags flags,
			     void* userdata)
{
	AvahiClient *c;
	struct controller *ctrl = userdata;

	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		c = avahi_service_resolver_get_client(r);
		pr_err("(Resolver) Failed to resolve service"
			" '%s' of type '%s' in domain '%s': %s\n",
			name, type, domain,
			avahi_strerror(avahi_client_errno(c)));
		break;

	case AVAHI_RESOLVER_FOUND:
		tcpip_hotplug(ctrl, host_name, address, port);
		break;
	}

	avahi_service_resolver_free(r);
}

static void browse_callback(AvahiServiceBrowser *b,
			    AvahiIfIndex interface,
			    AvahiProtocol protocol,
			    AvahiBrowserEvent event,
			    const char *name,
			    const char *type,
			    const char *domain,
			    AvahiLookupResultFlags flags,
			    void* userdata)
{
	struct controller *ctrl = userdata;
	struct tcpip_controller *tcpip_ctrl = ctrl->priv;
	AvahiClient *c = tcpip_ctrl->client;
	AvahiServiceResolver *r;

	switch (event) {
	case AVAHI_BROWSER_FAILURE:
		c = avahi_service_browser_get_client(b);
		pr_err("(Browser) %s\n", 
			avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(tcpip_ctrl->simple_poll);
		return;

	case AVAHI_BROWSER_NEW:
		r = avahi_service_resolver_new(c, interface, protocol,
					       name, type, domain,
					       AVAHI_PROTO_UNSPEC, 0,
					       resolve_callback, userdata);
		if (!r) {
			pr_err("Failed to resolve service '%s': %s\n",
				name, avahi_strerror(avahi_client_errno(c)));
		}

		return;

	case AVAHI_BROWSER_REMOVE:
		/* TODO */
		return;

	default:
		return;
	}
}

static void client_callback(AvahiClient *c,
			    AvahiClientState state, void *userdata)
{
	struct controller *ctrl = userdata;
	struct tcpip_controller *tcpip_ctrl = ctrl->priv;

	if (state == AVAHI_CLIENT_FAILURE) {
		pr_err("Server connection failure: %s\n",
			avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(tcpip_ctrl->simple_poll);
	}
}

static void tcpip_intf_destroy(struct interface *intf)
{
}

static int avahi_discovery(struct controller *ctrl)
{
	AvahiClient *client;
	AvahiServiceBrowser *sb;
	AvahiSimplePoll *simple_poll;
	struct tcpip_controller *tcpip_ctrl = ctrl->priv;
	int ret = 0;
	int error;

	simple_poll = avahi_simple_poll_new();
	if (!simple_poll) {
		pr_err("Failed to create simple poll object\n");
		return -ENOMEM;
	}

	client = avahi_client_new(avahi_simple_poll_get(simple_poll),
				  0, client_callback, ctrl, &error);
	if (!client) {
		ret = error;
		pr_err("Failed to create client: %s\n", avahi_strerror(error));
		goto err_simple_pool_free;
	}

	tcpip_ctrl->client = client;
	sb = avahi_service_browser_new(client,
				       AVAHI_IF_UNSPEC, AVAHI_PROTO_INET,
				       "_greybus._tcp", NULL, 0,
				       browse_callback, ctrl); 
	if (!sb) {
		ret = avahi_client_errno(client);
		pr_err("Failed to create service browser: %s\n",
			avahi_strerror(avahi_client_errno(client)));
		goto err_client_free;
	}

	tcpip_ctrl->simple_poll = simple_poll;
	avahi_simple_poll_loop(simple_poll);

	avahi_service_browser_free(sb);
err_client_free:
	avahi_client_free(client);
err_simple_pool_free:
	avahi_simple_poll_free(simple_poll);

	return ret;
}

static void avahi_discovery_stop(struct controller *ctrl)
{
	struct tcpip_controller *tcpip_ctrl = ctrl->priv;

	avahi_simple_poll_quit(tcpip_ctrl->simple_poll);
}

static int tcpip_write(struct connection *conn, void *data, size_t len)
{
	struct tcpip_connection *tconn = conn->priv;

	return write(tconn->sock, data, len);
}

static int tcpip_read(struct connection *conn, void *data, size_t len)
{
	struct tcpip_connection *tconn = conn->priv;

	return read(tconn->sock, data, len);
}

static int tcpip_init(struct controller *ctrl)
{
	struct tcpip_controller *tcpip_ctrl;

	tcpip_ctrl = malloc(sizeof(*tcpip_ctrl));
	if (!tcpip_ctrl)
		return -ENOMEM;
	 ctrl->priv = tcpip_ctrl;

	return 0;
}

static void tcpip_exit(struct controller *ctrl)
{
	free(ctrl->priv);
}


struct controller tcpip_controller = {
	.name = "TCP/IP",
	.init = tcpip_init,
	.exit = tcpip_exit,
	.connection_create = tcpip_connection_create,
	.connection_destroy = tcpip_connection_destroy,
	.event_loop = avahi_discovery,
	.event_loop_stop = avahi_discovery_stop,
	.write = tcpip_write,
	.read = tcpip_read,
	.interface_destroy = tcpip_intf_destroy,
};