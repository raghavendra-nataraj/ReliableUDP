/*
 * Server.c
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */
#include "Server.h"
// pointer to sliding window locations
SlidingWindow *start,*acked,*sent;
// congession window
int cwnd = 1;
int cwndRecv = 0;
// final window size (always minimum of congession window and received window)
int fwnd = 0;
// received window
int rwnd = 0;
// variabe to switch between slow start and Congession avoidance
int isSlow =1;
// time out value caculation variables
int timeOut = 1;
long estRtt = 0;
long devRtt = 0;
// threshold values
int ssthresh = 64;

// count of packets at different situation
int packetsSS=0;
int packetsCA=0;
int packetsDrp = 0;
int packetsRet = 0;

// mutex to lock the sending packets
pthread_mutex_t sendLockMutex = PTHREAD_MUTEX_INITIALIZER;
// mutex to lock the sliding window
pthread_mutex_t slidingLockMutex = PTHREAD_MUTEX_INITIALIZER;

// pthread condition to block and open the sliding window
pthread_cond_t slidingCondThread = PTHREAD_COND_INITIALIZER;
pthread_cond_t slidingCond1Thread = PTHREAD_COND_INITIALIZER;

// minimum of two number
int min(int a,int b){
	int result = a<b?  a: b;
	return result;
}


void main(int argc,char *argv[]){
	int sockFd,portNum;
	pthread_t serverThread;
	int th1;
	struct sockaddr_in serv_addr;
	if(argc<3){
		error("Required port number & advertised Window");
	}
	sockFd =  socket(AF_INET,SOCK_DGRAM,0);
	if(sockFd<0){error("Error Opening Socket");}
	bzero((char*)&serv_addr,sizeof(serv_addr));
	portNum  = atoi(argv[1]);
	rwnd = atoi(argv[2]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portNum);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sockFd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){error("unable to bind");}
	listen(sockFd,5);
	while(1){
		th1 = pthread_create(&serverThread,NULL,&handleClient,(void*)&sockFd);
		pthread_join( serverThread, NULL);
		//handleClient((void*)&sockFd);
	}
}

// copy the server thread details to another server thread details
ServerThreadDetails *copyThreadDetails(ServerThreadDetails *details){
	ServerThreadDetails * temp = (ServerThreadDetails*)malloc(sizeof(ServerThreadDetails));
	temp->sockAddr = details->sockAddr;
	temp->sockFD = details->sockFD;
	temp->slidingNode = details->slidingNode;
	return temp;
}


// update the acked pointer
void updateAcked(){
	SlidingWindow *temp = acked;
	while(temp!=NULL){
		if(temp->ackFlag==0){
			acked = temp;
			break;
		}
		temp = temp->next;
	}
}

// time out calculation
void calculateTimeOut(SlidingWindow *node){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long retTime = tv.tv_sec*1000000 + tv.tv_usec;
	long sampleRtt = retTime - node->timeSent;
	estRtt = (0.875*estRtt) + (0.125*sampleRtt);
	devRtt =  devRtt + (0.25 * (abs(sampleRtt-estRtt)-devRtt));
	int tempTimeOut = (estRtt + (4*devRtt))/1000000;
	timeOut = tempTimeOut>1?tempTimeOut:1;
	//printf("timeOut=%d,%d,%d,%d\n",timeOut,devRtt,estRtt,sampleRtt);

}

// update the ackflag. calculate the rtt. calclate the cwnd size
int updateAckFlag(TCPHeader *ack,ServerThreadDetails *details){
	SlidingWindow *temp = start;
	ServerThreadDetails *tempDetails = copyThreadDetails(details);
	while(temp!=NULL){
		if(ack->finFlag == 1 && ack->finFlag == temp->header->finFlag){
			#ifdef DEBUG
			printf("FinAcker\n");
			#endif /*DEBUG*/
			temp->ackFlag = 1;
			temp->ackReceived = temp->ackReceived+1;
		}
		if(ack->order == temp->header->order && ack->ackNum==temp->header->seqNum){
			SlidingWindow *toAckNode = temp->prev;
			if(toAckNode->ackFlag == 0){
				calculateTimeOut(toAckNode);			
			}
			toAckNode->ackFlag = 1;
			toAckNode->ackReceived = toAckNode->ackReceived+1;
			if(toAckNode->ackReceived>2){
				packetsDrp++;
				toAckNode->ackReceived=0;
				tempDetails->slidingNode = temp;
				printf("dupAck\n");
				sendPacket(tempDetails);
				ssthresh = cwnd/2;
				cwndRecv=0;
				cwnd = ssthresh +3;
				fwnd = min(cwnd,rwnd);
				
			}
			//acked = temp;
			cwndChange();
			int retVal =  unlockSlideWindow();
			updateAcked();
			return retVal;
		}
		temp = temp->next;
	}
	cwndChange();
	int retVal =  unlockSlideWindow(); 
	updateAcked();
	return retVal;
}

// calculate the congession window and the final window
void cwndChange(){
	if(isSlow){
		if(cwndRecv==cwnd){
			cwnd = cwnd*2;
			cwndRecv=0;
			fwnd = min(rwnd,cwnd);
		}else{
			cwndRecv++;
		}
	}else{
		if(cwndRecv==cwnd){
			cwnd = cwnd+1;
			cwndRecv=0;
			fwnd = min(rwnd,cwnd);
		}else{
			cwndRecv++;
		}
	}
}

//retransmit packet if timed out
void *reTransmit(void *threadDeatils){
	ServerThreadDetails *details = copyThreadDetails((ServerThreadDetails*)threadDeatils);
	SlidingWindow *temp = details->slidingNode;
	sleep(timeOut);
	if(temp->ackFlag==0){
		packetsRet++;
		ssthresh = cwnd/2;
		cwnd=1;
		cwndRecv=0;
		fwnd = min(rwnd,cwnd);
		isSlow = 1;
		printf("\nCongession Avoidance -> Slow Start\n");
		printf("\nRetransmitting\n");
		sendPacket(details);
	}
	return NULL;
}

// Slow start to congession avoidance switch
void slowToCa(){
	if(isSlow && cwnd>ssthresh){
		isSlow = 0;
		printf("\nSlow Start -> Congession Avoidance\n");
	}

}


// function to send packet
void sendPacket(ServerThreadDetails *threadDeatils){
	pthread_mutex_lock(&sendLockMutex);
	struct timeval tv;
	pthread_t reTransmitThread;
	int reTransmitThreadId;
	TCPHeaderSend sendMe;
	ServerThreadDetails *details = copyThreadDetails(threadDeatils);
	int newSockFd = details->sockFD;
	SlidingWindow *temp = details->slidingNode;
	struct sockaddr_in cli_addr = details->sockAddr;
	socklen_t clilen = sizeof(cli_addr);
	sendMe.header = *(temp->header);
	if(temp->isSent == 0){
		gettimeofday(&tv,NULL);
		temp->timeSent = tv.tv_sec*1000000 + tv.tv_usec;
	}
	if(isSlow){packetsSS++;}
	else{packetsCA++;}
	temp->isSent = 1;
	copyString(sendMe.data,temp->header->data,strlen(temp->header->data));
	sendto(newSockFd,&sendMe,sizeof(TCPHeaderSend),0,(struct sockaddr*)&cli_addr,clilen);
	slowToCa();
	#ifdef DEBUG
	printf("\nsent = %lu\n",temp->header->seqNum);
	#endif /*DEBUG*/
	reTransmitThreadId = pthread_create(&reTransmitThread,NULL,&reTransmit,(void*)threadDeatils);
	pthread_mutex_unlock(&sendLockMutex);

}


// calculate when the fin packet can be sent
int canSendFinPacket(){
	SlidingWindow *temp = start;
	while(temp->next!=NULL){
		if(temp->ackFlag==0)
			return 0;
		temp = temp->next;
	}
	return 1;
}

// calculate when the sliding window can be locked
int lockSlideWindow(){
	SlidingWindow *temp = acked;
	while(temp!=sent){
		if(temp->ackFlag==1)
			return 0;
		temp = temp->next;
	}
	return 1;
}

// release the lock on sliding window after all packets are transfered
void releaseLockOnRecv(){
	pthread_mutex_lock(&slidingLockMutex);
	pthread_cond_signal(&slidingCond1Thread);
	pthread_mutex_unlock(&slidingLockMutex);
}

// function to send packets that are not sent in the sliding window
void *sendPackets(void *threadDeatils){
	int i;
	ServerThreadDetails *details = copyThreadDetails((ServerThreadDetails*)threadDeatils);
	int newSockFd = details->sockFD;
	struct sockaddr_in cli_addr = details->sockAddr;
	socklen_t clilen = sizeof(cli_addr);
	while(1){
		SlidingWindow *temp = acked;
		for(i =0;i<fwnd && temp!=NULL;i++){
			#ifdef DEBUG
			printf("%lu ",temp->header->seqNum);
			#endif
			if(temp->isSent==1){
			temp = temp->next;
			if(temp->next!=NULL)
				sent = temp;
			continue;}
			details->slidingNode = temp;
			if(temp->header->finFlag==1){
				if(canSendFinPacket()==1){
					#ifdef DEBUG	
					printf("fin'd\n\n\n");
					#endif /*DEBUG*/
					sendPacket(details);
					releaseLockOnRecv();
					return NULL;
				}
			}else{
				sendPacket(details);
			}
			temp = temp->next;
			if(temp!=NULL){
				sent = temp;
			}
		}
		if(lockSlideWindow()){
			pthread_mutex_lock(&slidingLockMutex);	
			pthread_cond_signal(&slidingCond1Thread);
			pthread_cond_wait(&slidingCondThread,&slidingLockMutex);
			pthread_mutex_unlock(&slidingLockMutex);
		}
		#ifdef DEBUG
		printf("\n");
		#endif /*DEBUG*/
	}	
	return NULL;
}

// finction to calcukate when to release the sliding window
int unlockSlideWindow(){
	SlidingWindow *temp = acked;
	while(temp!=NULL){
		if(temp->ackFlag==1){
			return 1; 
		}
		temp = temp->next;
	} 
	return 0;
}

// function to receive the ack and update the structure fields
void *recieveAck(void *threadDeatils){
	ServerThreadDetails *details = copyThreadDetails((ServerThreadDetails*)threadDeatils);
	int newSockFd = details->sockFD;
	struct sockaddr_in cli_addr = details->sockAddr;
	char *buffer = (char*)malloc(sizeof(TCPHeader));
	socklen_t clilen = sizeof(cli_addr);
	while(1){
		//pthread_mutex_lock(&slidingLockMutex);
		recvfrom(newSockFd,buffer,sizeof(TCPHeader),0,(struct sockaddr*)&cli_addr,&clilen);
		TCPHeader *ackNode = (TCPHeader*)buffer;	
		if(updateAckFlag(ackNode,details)){
			if(ackNode->finFlag==1){
				pthread_mutex_lock(&slidingLockMutex);
				pthread_cond_signal(&slidingCondThread);
				pthread_mutex_unlock(&slidingLockMutex);
				#ifdef DEBUG
				printf("recv =%lu,%d\n",ackNode->ackNum,ackNode->finFlag);
				#endif/*DEBUG*/
				return NULL;
			}
			pthread_mutex_lock(&slidingLockMutex);
			pthread_cond_signal(&slidingCondThread);
			pthread_cond_wait(&slidingCond1Thread,&slidingLockMutex);
			pthread_mutex_unlock(&slidingLockMutex);
		}
		//if(ackNode->finFlag==1){
		//	return NULL;
		//}
		#ifdef DEBUG
		printf("recv =%lu,%d\n",ackNode->ackNum,ackNode->finFlag);
		#endif /*DEBUG*/
	}
	return NULL;
}

// function to receive the file name
char *recieveRequest(int newSockFd,struct sockaddr_in *cli_addr){
	char *buffer = (char*)calloc(100,sizeof(char));
	socklen_t clilen = sizeof(*cli_addr);
	recvfrom(newSockFd,buffer,100,0,(struct sockaddr*)cli_addr,&clilen);
	return buffer;
}

// thread to handle clients 
void *handleClient(void *sockFd){
	int newSockFd = *((int*)sockFd);
	pthread_t sendThread,recvThread;
	int thSend,thRecv;
	struct sockaddr_in serv_addr,cli_addr;
	cwnd = 1;
	cwndRecv =0;
	ssthresh = 64;
	fwnd = min(cwnd,rwnd);
	isSlow = 1;
	estRtt = 0;
	devRtt = 0;
	timeOut = 1;
	packetsSS=0;
	packetsCA=0;
	packetsDrp = 0;
	packetsRet = 0;
	pthread_mutex_init(&sendLockMutex,NULL);
	pthread_mutex_init(&slidingLockMutex,NULL);
	pthread_cond_init(&slidingCondThread,NULL);
	pthread_cond_init(&slidingCond1Thread,NULL);
	char *fileName = recieveRequest(newSockFd,&cli_addr);
	char *fileContent = readFile(fileName);
	TCPHeader *node = createTCPPack4FileContents(fileContent);
	SlidingWindow *slidingNode = createSlidingWindow4TCPPackets(node);
	start = acked = sent = slidingNode;
	ServerThreadDetails details;
	details.slidingNode =slidingNode;
	details.sockFD = newSockFd;
	details.sockAddr = cli_addr;
	//sendPackets((void*)&details);
	thSend = pthread_create(&sendThread,NULL,&sendPackets,(void*)&details);
	//pthread_join( sendThread, NULL);
	thRecv = pthread_create(&recvThread,NULL,&recieveAck,(void*)&details);
	//thSend = pthread_create(&sendThread,NULL,&sendPackets,(void*)&details);
	//void *ack;
	pthread_join( sendThread, NULL);
	pthread_join( recvThread, NULL);
	deleteWindow(slidingNode);
	printf("\nfile transfer Completed\n");	
	int totPack = packetsSS + packetsCA;
	printf("Total packets transfered = %d\n",totPack);
	printf("Packets transfered in slow Start = %d\n",packetsSS);	
	printf("Packets transfered in CA = %d\n",packetsCA);		
	printf("Packets Dupilicate Acked = %d\n",packetsDrp);	
	printf("Packets Timed out = %d\n",packetsRet);	
	printf("Percent of Packets transfered in Slow Start = %f\n",(packetsSS/(float)totPack)*100);
	printf("Percent of Packets transfered in Congession Avoidance = %f\n",(packetsCA/(float)totPack)*100);
	printf("Percent of Packets Duplicate Acked = %f\n",(packetsDrp/(float)totPack)*100);
	printf("Percent of Packets Timed Out = %f\n",(packetsRet/(float)totPack)*100);
	
	return NULL;
}
