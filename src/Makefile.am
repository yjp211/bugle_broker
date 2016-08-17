AM_CFLAGS = -g  -O2 -I../include  -I../static-lib/include  -I ../evws

bin_PROGRAMS = broker 

broker_LDADD = -lpthread -ljson-c -levent  -lcurl -lnettle -lwslay 

EVWS_OBJECTS = ../evws/evws_util.c ../evws/evws.c ../evws/http_parser.c ../evws/wslistener.c

broker_SOURCES = $(EVWS_OBJECTS) config_opt.c map.c misc.c conn.c mqtt.c pool.c log.c topic.c common_handle.c common_process.c broker_handle.c broker_process.c rpc_handle.c rpc_process.c websockets.c broker.c

