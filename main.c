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
#include "controllers/uart.h"

int run;

static int gbridge_greybus_send_request(uint8_t intf_id, uint16_t cport_id,
					void *data, size_t len)
{
	return controller_write(intf_id, cport_id, data, len);
}

static int gbridge_greybus_send_response(uint8_t intf_id, uint16_t cport_id,
					 void *data, size_t len)
{
	struct connection *conn;

	conn = get_connection(intf_id, cport_id);
	if (!conn)
		return -EINVAL;

	return controller_write(conn->intf1->id, conn->cport1_id, data, len);
}

static struct greybus_driver *gbridge_greybus_driver(uint8_t intf_id,
						     uint16_t cport_id)
{
	struct interface *intf;

	intf = get_interface(intf_id);
	if (!intf) {
		pr_err("Invalid interface id %d\n", intf_id);
		return NULL;
	}

	return intf->gb_drivers[cport_id];
}

static int gbridge_greybus_register_driver(uint8_t intf_id, uint16_t cport_id,
					   struct greybus_driver *driver)
{
	struct interface *intf;

	intf = get_interface(intf_id);
	if (!intf) {
		pr_err("Invalid interface id %d\n", intf_id);
		return -EINVAL;
	}

	intf->gb_drivers[cport_id] = driver;

	return 0;
}

static struct greybus_platform gbridge_platform = {
	.greybus_send_request = gbridge_greybus_send_request,
	.greybus_send_response = gbridge_greybus_send_response,
	.greybus_driver = gbridge_greybus_driver,
	.greybus_register_driver = gbridge_greybus_register_driver,
};

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
	run = 0;
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

	register_controllers();

	while ((c = getopt(argc, argv, "p:b:m:")) != -1) {
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
		case 'm':
#ifdef GBSIM
			ret = register_gbsim_controller(optarg);
			if (ret)
				return ret;
			break;
#else
			pr_err("You must build gbridge with gbsim enabled\n");
			return -EINVAL;
#endif
		default:
			help();
			return -EINVAL;
		}
	}

	ret = greybus_init(&gbridge_platform);
	if (ret) {
		pr_err("Failed to init Greybus\n");
		return ret;
	}

	if (uart) {
		ret = register_uart_controller(uart, baudrate);
		if (ret)
			return ret;
	}

	run = 1;
	controllers_init();
	while(run)
		sleep(1);
	controllers_exit();

	return 0;
}
