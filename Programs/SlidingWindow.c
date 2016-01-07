/*
 * SlidingWindow.c
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */
#include "SlidingWindow.h"

/* create sliding window node*/
SlidingWindow * createSlidingNode(){
	SlidingWindow *tempNode = (SlidingWindow*)malloc(sizeof(SlidingWindow));
	tempNode->ackFlag=0;
	tempNode->ackReceived = 0;
	tempNode->cwnd = 0;
	tempNode->isSent = 0;
	tempNode->timeSent = 0;
	tempNode->next = NULL;
	tempNode->prev = NULL;
	return tempNode;
}

/* create a linket list of sliding window from the linked list of TCPheader to handle sliding window concept*/
SlidingWindow *createSlidingWindow4TCPPackets(TCPHeader *fileList){
	SlidingWindow *startNode=NULL;
	SlidingWindow *prevNode=NULL;
	unsigned long seqNumber=0, tempBufferLen,packetSize;
	TCPHeader *tempTCPNode = fileList;
	while(tempTCPNode!=NULL){
		SlidingWindow *node = createSlidingNode();
		node->header = tempTCPNode;
		if(startNode == NULL){
			startNode = node;
			prevNode = node;
		}else{
			prevNode->next = node;
			node->prev = prevNode;
			prevNode = node;
		}
		tempTCPNode = tempTCPNode->next;
	}
	return startNode;
}

/* extract and link the tcpheaders from the received and buffered sliding window linket list.*/
TCPHeader *extractSlidingWindow4TCPPackets(SlidingWindow *slidingList){
	SlidingWindow *tempNode = slidingList;
	TCPHeader *startNode = NULL;
	TCPHeader *currentNode = NULL;
	TCPHeader *prevNode = NULL;
	while(tempNode!=NULL){
		if(startNode ==NULL){
			startNode = tempNode->header;
			currentNode = tempNode->header;
			prevNode = tempNode->header;
		}else{
			currentNode = tempNode->header;
			currentNode->prev = prevNode;
			prevNode->next = currentNode;
			prevNode = currentNode;
		}
		tempNode = tempNode->next;
	}
#ifdef DEBUG1
	TCPHeader * tempStartNode = startNode;
	while(tempStartNode!=NULL){
		if(tempStartNode->data!=NULL)
			printf("%s",tempStartNode->data);
		tempStartNode = tempStartNode->next;
	}
#endif /*DEBUG*/
	return startNode;
}


/* appends in order and out of order tcpheader to the linked list.*/
 SlidingWindow *appendslidingWindow(SlidingWindow *startNode,TCPHeader *tempNode,char *buffer){
	SlidingWindow *prevNode= NULL;
	SlidingWindow *tempSliding = createSlidingNode();
	tempSliding->header = copyTCPNode(tempNode);
	copyString(tempSliding->header->data,buffer,strlen(buffer));	
	if(startNode==NULL){
		startNode = tempSliding;
		prevNode = tempSliding;
	}else{
			SlidingWindow *temp = startNode;
		while(temp!=NULL){
			if((temp->header->order == tempSliding->header->order && temp->header->seqNum<tempSliding->header->seqNum)|| temp->header->order < tempSliding->header->order){
				
				prevNode = temp;
				temp = temp->next;
				continue;				
			}else{
				if(temp->header->order == tempSliding->header->order && temp->header->seqNum==tempSliding->header->seqNum){
					return startNode;
				}
				SlidingWindow *beforeNode = temp->prev;
				SlidingWindow *afterNode = temp;
				if(beforeNode==NULL){
					startNode = tempSliding;
					tempSliding->next = afterNode;
					afterNode->prev = tempSliding;
					return startNode;
				}
				beforeNode->next = tempSliding;
				tempSliding->prev = beforeNode;
				afterNode->prev = tempSliding;
				tempSliding->next = afterNode;
				return startNode;
			}
		}
		prevNode->next = tempSliding;
		tempSliding->prev = prevNode;
	}
	return startNode;
}

/* deletes the sliding window linked list*/
void deleteWindow(SlidingWindow *startNode){
	while(startNode!=NULL){
		SlidingWindow *temp = startNode;
		startNode = startNode->next;
		free(temp->header->data);
		free(temp->header);
		free(temp);
		temp = NULL;
	}
}

