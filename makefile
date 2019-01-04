CC = gcc
basic = -Wall

ifdef DEBUG
debug = $(basic) -D DEBUG
else
debug = $(basic)
endif

ifdef GDB
FLAGS = $(debug) -ggdb
else
FLAGS = $(debug)
endif

$(info FLAGS set to: $(FLAGS))

help:
	@echo ""
	@echo "Usage: make [flag settings] <--> [target]"
	@echo "flag settings:"
	@echo "DEBUG	set to any value (e.g. 'DEBUG=1') for verbose executables"
	@echo "GDB	set to any value for attaching debugging info for gdb"
	@echo ""
	@echo "target:"
	@echo "all:	tftp_server and tftp_client"
	@echo "tftp_server:	TFTP server"
	@echo "tftp_client:	TFTP client"
	@echo "clean:		remove objects and executables"


all: tftp_server tftp_client


#%_test: test/%_test.c
#	$(CC) $(FLAGS) $< -o testbin/$@ > log/compilation_$@.log 2> log/compilation_$@.err


objects = obj/pack_message.o obj/tftp_send.o obj/tftp_recv.o obj/tftp_message_alloc.o obj/tftp_request_exchange_buffer.o obj/tftp_display.o

tftp_server : $(objects) obj/tftp_server.o
	if [ ! -e bin/ ] ; then mkdir bin ; fi
	$(CC) $(FLAGS) -lpthread obj/tftp_server.o $(objects) -o bin/$@

obj/tftp_server.o : tftp_server.c headers/tftp_display.h headers/pack_message.h headers/tftp_message_alloc.h headers/tftp_constants.h headers/tftp_messages.h headers/tftp_recv.h headers/tftp_send.h headers/tftp_request_exchange_buffer.h
	$(CC) $(FLAGS) -c tftp_server.c -o $@

tftp_client : $(objects) obj/tftp_client.o
	if [ ! -e bin/ ] ; then mkdir bin ; fi
	$(CC) $(FLAGS) obj/tftp_client.o $(objects) -o bin/$@

obj/tftp_client.o : tftp_client.c headers/pack_message.h headers/tftp_message_alloc.h headers/tftp_constants.h headers/tftp_messages.h headers/tftp_recv.h headers/tftp_send.h headers/tftp_request_exchange_buffer.h
	$(CC) $(FLAGS) -c tftp_client.c -o $@


obj/%.o : lib/%.c
	if [ ! -e obj/ ] ; then mkdir obj ; fi
	$(CC) $(FLAGS) -c $< -o $@

.PHONY: clean
clean:
	if [ -e obj/ ] ; then rm -r obj ; fi
	if [ -e bin/ ] ; then rm -r bin ; fi
