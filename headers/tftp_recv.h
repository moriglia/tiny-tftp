#ifndef TFTP_RECV_H
#define TFTP_RECV_H

int tftp_recv(int sockfd, struct tftp_message* const message,
	      struct sockaddr* src_addr, socklen_t *addrlen);

#endif
