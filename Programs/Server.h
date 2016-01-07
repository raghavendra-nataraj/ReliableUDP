/*
 * Server.h
 *
 *  Created on: Oct 1, 2015
 *      Author: ragha
 */

#ifndef SERVER_H_
#define SERVER_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include "Helper.h"
#include "TCPHeader.h"
#include "SlidingWindow.h"


// pointer to pass in thread creation
typedef struct ServerThreadDetails{
	struct sockaddr_in sockAddr; // receivers address
	SlidingWindow *slidingNode;
	int sockFD; // socket ID
}ServerThreadDetails;

int  min(int,int);
void *reTransmit(void *);
void sendPacket(ServerThreadDetails *);
int canSendFinPacket();
void *sendPackets(void*);
void *reTransmit(void *);
int updateAckFlag(TCPHeader *ack,ServerThreadDetails*);
int unlockSlidingWindow();
void cwndChange();
char *recieveRequest(int ,struct sockaddr_in *);
void *recieveAck(void *);
void *handleClient(void *);
ServerThreadDetails *copyThreadDetails(ServerThreadDetails *);

#endif /* SERVER_H_ */
