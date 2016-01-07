/*
 * Helper.c
 *
 *  Created on: Sep 30, 2015
 *      Author: ragha
 */
#include <stdio.h>
#include <stdlib.h>
#include "Helper.h"

// print errors
void error(const char* msg){
	perror(msg);
	exit(0);
}

// copy contents of a file to a char pointer
char *readFile(char *fileName){
	long size;
	printf("FileName = %s\n",fileName);
	char *fileContent = NULL;
	FILE *fp = fopen(fileName,"rb");
	if(fp!=NULL){
		fseek(fp,0L,SEEK_END);
		size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		fileContent = (char*)calloc(size,sizeof(char));
		fread(fileContent,sizeof(char),size,fp);
		fclose(fp);
	}else{
		error(fileName);
	}
	return fileContent;
}

// write file contents to a file
void writeFile(char *contents,char *fileName){
	FILE *fp1 = fopen(fileName,"wb");
		if(fp1!=NULL){
			fwrite(contents,sizeof(char),strlen(contents),fp1);
			fclose(fp1);
		}else{
			error("big_retr.txt");
	}
}

// similar to memcpy with null terminating string
void copyString(char *dest,char* src,long length){
	int i;
	for(i=0;i<length;i++){
		dest[i] = src[i];
	}
	dest[length] = '\0';
}
