/*
 * TCPHeader.c
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */

#include "TCPHeader.h"

/*Create a TCP header Node*/
TCPHeader * createTCPNode(){
	TCPHeader *tempNode = (TCPHeader*)malloc(sizeof(TCPHeader));
	tempNode->finFlag = 0;
	tempNode->ackFlag = 0;
	tempNode->seqNum = 0;
	tempNode->order = 0;
	tempNode->ackNum = 0;
	tempNode->next = NULL;
	tempNode->data = NULL;
	tempNode->prev = NULL;
	return tempNode;
}

/*Create a TCP FIN header Node*/
TCPHeader *createTCPFinNode(){
	TCPHeader *tempNode = createTCPNode();
	tempNode->finFlag = 1;
	return tempNode;
}

/*Create a TCP ACK header Node*/
TCPHeader *createTCPAckNode(){
	TCPHeader *tempNode = createTCPNode();
	tempNode->ackFlag = 1;
	return tempNode;
}

/*Create a TCP FIN ACK header Node*/
TCPHeader *createTCPFinAckNode(){
	TCPHeader *tempNode = createTCPFinNode();
	tempNode->ackFlag = 1;

	return tempNode;
}

/*Creates a copy of TCPNode*/
TCPHeader *copyTCPNode(TCPHeader *node){
		TCPHeader *tempNode = createTCPNode();
		tempNode->ackFlag = node->ackFlag;
		tempNode->seqNum = node->seqNum;
		tempNode->ackNum = node->ackNum;
		tempNode->order = node->order;
		tempNode->finFlag = node->finFlag;
		tempNode->data = calloc(1001,sizeof(char));
		return tempNode;
}

/*This function breaks the file contents into different packets
and creates a linked list of the packets teminated by FIN*/
TCPHeader *createTCPPack4FileContents(char *fileContents){
	TCPHeader *startNode=NULL;
	TCPHeader *prevNode = NULL;
	int order = 0;
	unsigned long seqNumber = 0;
	// near the max value of long. Used to exhibit wrap up protection.
	#ifdef WRAPUP
	seqNumber=18446744073709540000;
	#endif /*WRAPUP*/
	unsigned long tempBufferLen=0,packetSize=0;
	char *buffer = fileContents;
	while(buffer!=NULL && strlen(buffer)!=0){
		TCPHeader *node = createTCPNode();
		node->data = (char*)calloc(1001,sizeof(char));
		node->ackFlag = 0;
		node->ackNum = 0;
		node->order = order;
		node->seqNum = seqNumber;
		strncpy(node->data,buffer,1000);
		tempBufferLen = strlen(node->data);
		buffer+=tempBufferLen;
		packetSize = tempBufferLen;//tempBufferLen>=1000?1000:tempBufferLen;		
		seqNumber=(seqNumber + packetSize)%ULONG_MAX;
		if(seqNumber<node->seqNum){
			order++;
		}
		if(startNode == NULL){
			startNode = node;
			prevNode = node;
		}else{
			prevNode->next = node;
			node->prev = prevNode;
			prevNode = node;
		}
	}
	// attach fin packet
	TCPHeader *finNode =createTCPFinNode();
	finNode->seqNum = seqNumber;
	finNode->data = (char*)calloc(1001,sizeof(char));
	finNode->order = order;
	prevNode->next = finNode;
	finNode->prev = prevNode;
	prevNode = finNode;
	return startNode;
}

// extract data from the TCP packets and return a file content buffer.
char *extractTCPPack4FileContents(TCPHeader *fileList){
	int size=0;
	TCPHeader *tempNode = fileList;
	while(tempNode!=NULL){
		size = size+strlen(tempNode->data);
		tempNode = tempNode->next;
	}
	char *fileContent = (char *)(calloc(size,sizeof(char)));
	tempNode = fileList;
	while(tempNode!=NULL){
		if(tempNode->data!=NULL)
			strncat(fileContent,tempNode->data,strlen(tempNode->data));	
		tempNode = tempNode->next;
	}
	return fileContent;
}
//void deleteHeader(TCPHeader*);
