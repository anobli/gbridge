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
#include <pthread.h>

#include <debug.h>
#include <gbridge.h>
#include <controller.h>

#include <netlink/genl/mngt.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

static struct nl_sock *sock;
static pthread_t nl_recv_thread;
static struct nla_policy gb_nl_policy[GB_NL_A_MAX + 1] = {
	[GB_NL_A_DATA] = {.type = NLA_BINARY,.maxlen = GB_NETLINK_MTU},
	[GB_NL_A_CPORT] = {.type = NLA_U32},
};

static int
parse_gb_nl_msg(struct nl_cache_ops *unused, struct genl_cmd *cmd,
		struct genl_info *info, void *arg)
{
	struct gb_operation_msg_hdr *hdr;
	uint16_t hd_cport_id;
	size_t len;
	int ret;

	if (!info->attrs[GB_NL_A_DATA] || !info->attrs[GB_NL_A_CPORT])
		return -EPROTO;

	hd_cport_id = nla_get_u32(info->attrs[GB_NL_A_CPORT]);
	len = nla_len(info->attrs[GB_NL_A_DATA]);
	hdr = nla_data(info->attrs[GB_NL_A_DATA]);

	if (len < sizeof(*hdr)) {
		pr_err("short message received\n");
		return -EPROTO;
	}

	if (hd_cport_id == SVC_CPORT) {
		ret = greybus_handler(hd_cport_id, hdr);
		if (ret) {
			pr_err("Failed to handle svc operation %d: %d\n",
			       hdr->type, ret);
		}
	} else {
		ret = controller_write(hd_cport_id, hdr, gb_operation_msg_size(hdr));
	}

	return 0;
}

static struct genl_cmd cmds[] = {
	{
		.c_id = GB_NL_C_MSG,
		.c_name = "gb_nl_msg()",
		.c_maxattr = GB_NL_A_MAX,
		.c_attr_policy = gb_nl_policy,
		.c_msg_parser = &parse_gb_nl_msg,
	},
};

#define ARRAY_SIZE(X) (sizeof(X) / sizeof((X)[0]))

static struct genl_ops ops = {
	.o_name = GB_NL_NAME,
	.o_cmds = cmds,
	.o_ncmds = ARRAY_SIZE(cmds),
};

int netlink_send(uint16_t hd_cport_id, void *data, size_t len)
{
	struct nl_data *nl_data;
	struct nl_msg *msg;
	void *hdr;
	int ret;

	msg = nlmsg_alloc();
	if (!msg) {
		pr_err("Failed to allocate netlink message\n");
		ret = -ENOMEM;
		goto err_msg_free;
	}

	hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ops.o_id,
			  0, 0, GB_NL_C_MSG, 0);
	if (!hdr) {
		pr_err("Failed to write header\n");
		ret = -ENOMEM;
		goto err_msg_free;
	}

	nl_data = nl_data_alloc(data, len);
	if (!nl_data) {
		pr_err("Failed to allocate data\n");
		ret = -ENOMEM;
		goto err_msg_free;
	}

	nla_put_u32(msg, GB_NL_A_CPORT, hd_cport_id);
	nla_put_data(msg, GB_NL_A_DATA, nl_data);
	ret = nl_send_auto(sock, msg);
	if (ret < 0)
		pr_err("Failed to send message: %s\n", nl_geterror(ret));

	nl_data_free(nl_data);

	return 0;

 err_msg_free:
	nlmsg_free(msg);

	return ret;
}

void *nl_recv_cb(void *data)
{
	int ret;

	while (1) {
		ret = nl_recvmsgs_default(sock);
		if (ret < 0) {
			pr_err("Failed to receive message: %s\n",
			       nl_geterror(ret));
		}
	}

	return NULL;
}

int events_cb(struct nl_msg *msg, void *arg)
{
	return genl_handle_msg(msg, arg);
}

int netlink_init(void)
{
	int ret;

	sock = nl_socket_alloc();
	nl_socket_set_local_port(sock, GB_NL_PID);
	nl_socket_disable_seq_check(sock);
	nl_socket_disable_auto_ack(sock);
	nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, events_cb, NULL);
	nl_connect(sock, NETLINK_GENERIC);

	ret = genl_register_family(&ops);
	if (ret < 0) {
		pr_err("Failed to register family\n");
		goto error;
	}

	ret = genl_ops_resolve(sock, &ops);
	if (ret < 0) {
		pr_err("Failed to resolve family name\n");
		goto error;
	}

	return pthread_create(&nl_recv_thread, NULL, nl_recv_cb, NULL);

 error:
	nl_close(sock);
	nl_socket_free(sock);

	return ret;
}

void netlink_loop(void)
{
	pthread_join(nl_recv_thread, NULL);
}

void netlink_cancel(void)
{
	pthread_cancel(nl_recv_thread);
}

void netlink_exit(void)
{
	nl_close(sock);
	nl_socket_free(sock);
}
