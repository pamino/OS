#ifndef LIB_H
#define LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "array.h"

void release(char*** pppArr);

int try_(int r, char *err);

static inline void die(const char* msg) {
	perror(msg);
	exit(-1);
}

void delete(char** ppArr, int pos);

// ----- relate ----
void relate(char* pOut, const char* pBase, const char* pOff);

// ----- split ----
void split(char delimiter, const char* pStr, char** ppRet);

// ----- translateDir ----
void translateDir(const char* pArgument, char* ppDir, char* pRet);

#endif
