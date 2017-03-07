bin_PROGRAMS = gbridge

gbridge_CFLAGS = -Wall -Werror -I${GBDIR}
gbridge_CFLAGS += `pkg-config --cflags libnl-3.0 libnl-genl-3.0`
gbridge_CFLAGS += `pkg-config --cflags bluez`

gbridge_SOURCES = main.c \
		  debug.c \
		  greybus.c \
		  controller.c \
		  protocols/svc.c

if NETLINK
gbridge_SOURCES += controllers/gb_netlink.c
endif

if TPCIP
gbridge_SOURCES += controllers/tcpip.c
endif

if BLUETOOTH
gbridge_SOURCES += controllers/bluetooth.c
endif

if UART
gbridge_SOURCES += controllers/uart.c
endif