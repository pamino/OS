#include "lib.h"

void debug() {
	static int i = 0;
	++i;
	printf("pos %i\n", i);
}

int try_(int r, char* err) {
	if (r == -1) {
		perror(err);
		exit(1);
	}
	return r;
}

void release(char*** pppArr) {
	for (int i = 0; i < arrayLen(*pppArr); ++i) arrayRelease((*pppArr)[i]);
	arrayClear(*pppArr);
}

void delete(char** ppArr, int pos) {
	for (int i = pos; i < arrayLen(ppArr) - 1; ++i) {
		ppArr[i] = ppArr[i + 1];
	}
	arrayRelease(ppArr[arrayLen(ppArr) - 1]);
	char* pDiscard = arrayPop(ppArr); ++pDiscard;
}

// ----- relate ----
void relate(char* pOut, const char* pBase, const char* pOff) {
	// 1: skip the shared prefix
	int shared = 0;
	for (; pOff[shared] == pBase[shared] && pOff[shared]; ++shared);
	if (pOff[shared] == pBase[shared]) {
		memcpy(pOut, "./", 3);
		return;
	}
	const char* ptr = pOff + shared;
	while (*--ptr != '/');

	shared = ptr - pOff + 1;
	int ascend = 0;
	for (const char* ptr = pBase + shared; (ptr = strchr(ptr, '/')); ++ptr, ++ascend);
	ascend *= 3;
	memmove(pOut + ascend, pOff + shared, strlen(pOff) - shared + 1);
	memset(pOut, '.', ascend);
	for (char* ptr = pOut; ascend > 0; ptr += 3, ascend -= 3) ptr[2] = '/';
}

// ----- split ----
void split(char delimiter, const char* pStr, char** ppRet) {
	char* pNewEle;
	arrayInit(pNewEle);
	arrayPush(ppRet) = pNewEle;
	int pos = 0;
	for (int i = 1; pStr[i]; ++i) {
		if (pStr[i] == delimiter) {
			arrayPush(ppRet[pos]) = '\0';
			char* pNewStr;
			arrayInit(pNewStr);
			arrayPush(ppRet) = pNewStr;
			++pos;
		}
		else {
			arrayPush(ppRet[pos]) = pStr[i];
		}
	}

	arrayPush(ppRet[pos]) = '\0';
}

// ----- translateDir ----
void translateDir(const char* pArgument, char* pDir, char* pRet) {
	strcpy(pRet, pDir);
	if (pArgument[0] != '/') strcat(pRet, "/");
	strcat(pRet, pArgument);

	char** ppDirs;
	arrayInit(ppDirs);
	split('/', pRet, ppDirs);

	for (int i = 0; i < arrayLen(ppDirs); ++i) {
		if (!strcmp(ppDirs[i], "..")) {
			if (i >= 1) {
				delete(ppDirs, i);
				delete(ppDirs, i - 1);
			}
		}
	}
	strcpy(pRet, "");
	strcat(pRet, "/");
	for (int i = 0; i < arrayLen(ppDirs); ++i) {
		char pTemp[arrayLen(ppDirs[i]) + 2];
		strcpy(pTemp, ppDirs[i]);
		if (i != arrayLen(ppDirs) - 1) strcat(pTemp, "/");
		strcat(pRet, pTemp);
	}
	release(&ppDirs);
}