lib_LIBRARIES = libgreybus.a
libgreybus_a_SOURCES = greybus/greybus.c
libgreybus_a_SOURCES += greybus/manifest.c
libgreybus_a_SOURCES += greybus/control.c
libgreybus_a_SOURCES += greybus/loopback.c
libgreybus_a_CFLAGS = -Iinclude -Wall -Werror -I${GBDIR}

bin_PROGRAMS = gbridge
gbridge_CFLAGS = -Wall -Werror -I${GBDIR} -I${GBNETLINKDIR}
gbridge_CFLAGS += `pkg-config --cflags libnl-3.0 libnl-genl-3.0`
gbridge_CFLAGS += `pkg-config --cflags bluez`
gbridge_LDADD = libgreybus.a

gbridge_SOURCES = main.c \
		  debug.c \
		  controller.c \
		  svc.c

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

if GBSIM
gbridge_SOURCES += controllers/gbsim.c
endif