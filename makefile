CC = gcc
FLAGS = -Wall -ggdb

all: tftp_server tftp_client


#%_test: test/%_test.c
#	$(CC) $(FLAGS) $< -o testbin/$@ > log/compilation_$@.log 2> log/compilation_$@.err


objects = obj/pack_message.o obj/tftp_send.o obj/tftp_recv.o obj/tftp_message_alloc.o obj/tftp_request_exchange_buffer.o

tftp_server : $(objects)
	$(CC) $(FLAGS) -lpthread obj/tftp_server.o $(objects) -o $@

obj/tftp_server.o : tftp_server.c headers/pack_message.h headers/tftp_message_alloc.h headers/tftp_constants.h headers/tftp_messages.h headers/tftp_recv.h headers/tftp_send.h headers/tftp_request_exchange_buffer.h
	$(CC) $(FLAGS) -c tftp_server.c -o $@

obj/%.o : lib/%.c
	if [ ! -e obj ] ; then mkdir obj ; fi
	$(CC) $(FLAGS) -c $< -o $@
