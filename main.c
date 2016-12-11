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
#include <signal.h>
#include <unistd.h>

#include <debug.h>
#include <controller.h>

#include "gbridge.h"
#include "netlink.h"
#include "uart.h"

static void help(void)
{
	printf("gbridge: Greybus bridge application\n"
		"\t-h: Print the help\n"
#ifdef HAVE_UART
		"uart options:\n"
		"\t-p uart_device: set the uart device\n"
		"\t-b baudrate: set the uart baudrate\n"
#endif
		);
}

static void signal_handler(int sig)
{
	netlink_cancel();
}

int main(int argc, char *argv[])
{
	int c;
	int ret;

	int baudrate = 115200;
	const char *uart = NULL;

	signal(SIGINT, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);

	while ((c = getopt(argc, argv, "p:b:")) != -1) {
		switch(c) {
		case 'p':
			uart = optarg;
			break;
		case 'b':
			if (sscanf(optarg, "%u", &baudrate) != 1) {
				help();
				return -EINVAL;
			}
			break;
		default:
			help();
			return -EINVAL;
		}
	}

	ret = greybus_init();
	if (ret) {
		pr_err("Failed to init Greybus\n");
		return ret;
	}

	ret = netlink_init();
	if (ret) {
		pr_err("Failed to init netlink\n");
		return ret;
	}

	ret = svc_init();
	if (ret) {
		pr_err("Failed to init SVC\n");
		goto err_netlink_exit;
	}

	if (uart) {
		ret = register_uart_controller(uart, baudrate);
		if (ret) {
			pr_err("Failed to init uart controller\n");
			goto err_netlink_exit;
		}
	}

	controllers_init();

	netlink_loop();

	controllers_exit();

	netlink_exit();

	return 0;

 err_netlink_exit:
	netlink_cancel();
	netlink_loop();
	netlink_exit();

	return ret;
}
