/*
 * SlidingWindow.h
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */

#ifndef SLIDINGWINDOW_H_
#define SLIDINGWINDOW_H_
#include "TCPHeader.h"

// dynamic header for sliding window
typedef struct slidingWindow{
	TCPHeader *header; /* pointer to TCP header and data */
	int ackReceived; /* number of acks received*/
	int isSent; /* flag to indicate if packet is sent */
	int ackFlag; /* flag to indicate if ack is received */
	long timeSent; /* varibale to calculate RTT*/
	int cwnd;
	struct slidingWindow *next;
	struct slidingWindow *prev;

}SlidingWindow;

SlidingWindow *createSlidingNode();
SlidingWindow *appendslidingWindow(SlidingWindow *,TCPHeader *,char *);
SlidingWindow *createSlidingWindow4TCPPackets(TCPHeader *fileList);
TCPHeader *extractSlidingWindow4TCPPackets(SlidingWindow *slidingList);
void deleteWindow(SlidingWindow*);
#endif /* SLIDINGWINDOW_H_ */
