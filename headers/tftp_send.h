#ifndef TFTP_SEND_H
#define TFTP_SEND_H

int tftp_send(int sockfd, const struct tftp_message* const message,
	      struct sockaddr_in* dest_addr, socklen_t addrlen);

#endif
