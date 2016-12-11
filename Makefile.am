bin_PROGRAMS = gbridge

gbridge_CFLAGS = -Wall -Werror -I${GBDIR}
gbridge_CFLAGS += `pkg-config --cflags libnl-3.0 libnl-genl-3.0`
gbridge_CFLAGS += `pkg-config --cflags bluez`

gbridge_SOURCES = main.c \
		  netlink.c \
		  debug.c \
		  greybus.c \
		  svc.c \
		  controller.c

if TPCIP
gbridge_SOURCES += tcpip.c
endif

if BLUETOOTH
gbridge_SOURCES += bluetooth.c
endif
