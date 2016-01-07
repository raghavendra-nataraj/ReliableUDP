/*
 * TCPHeader.h
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */

#ifndef TCPHEADER_H_
#define TCPHEADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

//Dynamic TCP header structure
typedef struct tcpHeader{
	unsigned long seqNum; /*sequence Number*/
	unsigned long ackNum; /*acknowledment Number*/
	int order; /*the order of packet(Used for sequence number wrap up protection)*/
	unsigned short ackFlag; /*ack flag*/
	unsigned short finFlag; /* fin flag to close the connection*/
	char *data; /* data pointer*/
	struct tcpHeader *next;
	struct tcpHeader *prev;
}TCPHeader;

//Static TCP header Structure
typedef struct tcpHeaderSend{
	TCPHeader header;
	char data[1001];
}TCPHeaderSend;

TCPHeader * createTCPNode();
TCPHeader * createTCPFinNode();
TCPHeader *createTCPAckNode();
TCPHeader *createTCPFinAckNode();
TCPHeader *copyTCPNode(TCPHeader *);
TCPHeader *createTCPPack4FileContents(char *fileContents);
char *extractTCPPack4FileContents(TCPHeader *);
void deleteHeader(TCPHeader*);

#endif /* TCPHEADER_H_ */
