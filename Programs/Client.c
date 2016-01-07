/*	
 * Client.c
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */

#include "Client.h"

long nxtSqnNum = 0;
int order = 0;
SlidingWindow *startNode = NULL;
SlidingWindow *toAckNode = NULL;

void main(int argc,char *argv[]){
	struct sockaddr_in server_addr;
	struct hostent *server;
	int sockFd,portNum,connType;
	if(argc<5){
		printf("Enter host name , port number , Src fileName & Dest fileName");
		exit(0);
	}
	sockFd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockFd<0){error("Error creating host");}
	portNum = atoi(argv[2]);
	server = gethostbyname(argv[1]);
	if(server==NULL){error("No such hosts");}
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portNum);
	bcopy((char*)server->h_addr,(char*)&server_addr.sin_addr.s_addr, server->h_length);
	requestFile(sockFd,argv[3],server_addr);	
	handleReadWrite(sockFd,server_addr,argv[4]);
}

// make request for the file to server
void requestFile(int newSockFd,char *fileName,struct sockaddr_in server_addr){
	socklen_t srvlen = sizeof(server_addr);
	sendto(newSockFd,fileName,strlen(fileName),0,(struct sockaddr*)&server_addr,srvlen);
}

// send acknowledgement to the server
void sendAck(SlidingWindow *node,int newSockFd,struct sockaddr_in server_addr){

	socklen_t srvlen = sizeof(server_addr);
	node->isSent = 1;
	TCPHeader *ackNode = createTCPAckNode();
	//ackNode->ackNum = tempNode->header.seqNum + strlen(tempNode->data);
	ackNode->ackNum = (node->header->seqNum + strlen(node->header->data))%ULONG_MAX;
	if(ackNode->ackNum<node->header->seqNum){
		ackNode->order = node->header->order+1;
	}else{
		ackNode->order = node->header->order;
	}
	sendto(newSockFd,ackNode,sizeof(TCPHeader),0,(struct sockaddr*)&server_addr,srvlen);
	#ifdef DEBUG
	printf("ack Num = %lu\n",ackNode->ackNum);
	#endif /*DEBUG*/
}

// calculate the ack pointer
void calculateAck(int newSockFd,struct sockaddr_in server_addr){
	SlidingWindow *temp = toAckNode->next;	
//	int nxtSqnNum = temp->header->seqNum+ strlen(temp->header->data);
//	temp = temp->next;
	while(temp!=NULL){	
		if(temp->header->seqNum !=nxtSqnNum){
			if(temp->header->order == order){
				sendAck(toAckNode,newSockFd,server_addr);
				break;
			}
		}
		if(temp->isSent == 0){	
			sendAck(temp,newSockFd,server_addr);
		}
		nxtSqnNum = (temp->header->seqNum + strlen(temp->header->data))%ULONG_MAX;
		if(nxtSqnNum<temp->header->seqNum){
			order++;
		}
		toAckNode = temp;
		temp = temp->next;

	}
}

// drop packet if already received
int dropPacket(SlidingWindow *start,TCPHeader recvdNode){

	SlidingWindow *temp = start;
	while(temp!=NULL){
		if(recvdNode.order == temp->header->order && recvdNode.seqNum == temp->header->seqNum){
			return 1;
		}
	temp = temp->next;
	}
	return 0;
}

/*void *retransmitAck(void *details){
	ServerThreadDetails *td = (ServerThreadDetails *)details;
	calculateAck(td->sockFD,td->sock_addr);
}*/

// handle send and receive of data
void handleReadWrite(int newSockFd,struct sockaddr_in server_addr,char *destFileName){	
	socklen_t srvlen = sizeof(server_addr);
	char *header = (char*)calloc(1,sizeof(TCPHeaderSend));
	while(1){
		bzero(header,sizeof(TCPHeaderSend));
		recvfrom(newSockFd,header,sizeof(TCPHeaderSend),0,(struct sockaddr*)&server_addr,&srvlen);
		TCPHeaderSend *tempNode = (TCPHeaderSend*)header;
		// section to simulate duplicate ack
		#ifdef DUPACK
		//srand(time(NULL));
		int randNum1 = rand();
		if(randNum1%107==0){
			printf("drop seqNum = %lu\n",tempNode->header.seqNum);
			continue;
		}
		#endif /*DUPACK*/

		//section to simlate high latency
		#ifdef TIMEOUT
		//srand(time(NULL));
		int randNum2 = rand();	
		if(randNum2%105==0){
			sleep(2);
		}
		#endif /*TIMEOUT*/
		#ifdef DEBUG
		printf("%lu\n",tempNode->header.seqNum);	
		#endif /*DEBUG*/
		if(dropPacket(startNode,tempNode->header)){goto CALC_ACK;}
		startNode = appendslidingWindow(startNode,&(tempNode->header),tempNode->data);
		if(toAckNode==NULL)
		{
			//pthread_t ackThread;
			//ServerThreadDetails details;
			//details.sockFD = newSockFd;
			//details.sock_addr = server_addr;
			toAckNode = startNode;
			nxtSqnNum = (toAckNode->header->seqNum+strlen(toAckNode->header->data))%ULONG_MAX;
			if(nxtSqnNum<toAckNode->header->seqNum){
				order++;
			}
			sendAck(toAckNode,newSockFd,server_addr);
			//pthread_create(&ackThread,NULL,&retransmitAck,&details);
		}
		//printf("%s",tempNode->data);
		if(tempNode->header.finFlag == 1){
			TCPHeader *ackNode = createTCPFinAckNode();
			ackNode->ackNum = tempNode->header.seqNum;
		//	ackNode->order = tempNode->header.order;
			sendto(newSockFd,ackNode,sizeof(TCPHeader),0,(struct sockaddr*)&server_addr,srvlen);
			#ifdef DEBUG
			printf("ack Num = %lu\n",tempNode->header.seqNum);
			#endif/*DEBUG*/
			break;
		}
		CALC_ACK:
		calculateAck(newSockFd,server_addr);
	}
	TCPHeader *recvSlidingNode = extractSlidingWindow4TCPPackets(startNode);
	
	char *fileContents = extractTCPPack4FileContents(recvSlidingNode);

	//printf("%s",fileContents);
	writeFile(fileContents,destFileName);
	deleteWindow(startNode);
}
