/*
 * Client.h
 *
 *  Created on: Oct 2, 2015
 *      Author: ragha
 */

#ifndef CLIENT_H_
#define CLIENT_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include "Helper.h"
#include "TCPHeader.h"
#include "SlidingWindow.h"


void handleReadWrite(int newSockFd,struct sockaddr_in server_addr,char *);
void requestFile(int ,char *,struct sockaddr_in);
void calculateAck(int,struct sockaddr_in );
#endif /* CLIENT_H_ */
