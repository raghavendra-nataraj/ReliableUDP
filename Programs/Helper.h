/*
 * Helper.h
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */

#ifndef HELPER_H_
#define HELPER_H_
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

void error(const char *);
char *readFile(char *fileName);
void writeFile(char *contents,char *fileName);
void copyString(char * ,char *,long);

#endif /* HELPER_H_ */
