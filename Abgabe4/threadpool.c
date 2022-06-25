#include "threadpool.h"

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "array.h"

static Future** pJobQueue;
static pthread_mutex_t mJobQueue;

static Future** pJobDone;
static pthread_mutex_t mJobDone;

// static pthread_t* pThreads;
// static pthread_mutex_t mThreads;

static pthread_t main;
static sem_t end;

int tpInit(size_t size) {
	uint numThreads = 0;
	numThreads = (uint) size;
	sem_init(&end, 0, 0);

	pthread_mutex_init(&mJobQueue, NULL);
	arrayInit(pJobQueue);
	pthread_create(&main, NULL, managerThread, &numThreads);
	return 0;
}

void tpRelease(void) {
	sem_post(&end);

	arrayRelease(pJobDone);
	arrayRelease(pJobQueue);
	//arrayRelease(pThreads);

	sem_destroy(&end);
}

void tpAsync(Future* pFuture) {
	push(*pFuture);
}

void tpAwait(Future* pFuture) {
	while (1) {
		if (searchDone(pFuture)){
			popDone(pFuture);
			return;
		}
		else {
			int onlyOneJob = 1;
			workerThread((void*)&onlyOneJob);
		}
	}
}

void serialize(Future* fut) {
	static atomic_int counter;
	++counter;
	fut->id = counter;
}


void* workerThread(void* arg) {
	Future* job;
	while (1) {
		if (job = popJob()) {
			serialize(job);

			pthread_t t;
			pthread_create(t, NULL, job->fn, job);
			pthread_join(t, NULL);
			pushDone(job);
		}
		else {
			nsleep(1000);
		}
		if (*(int*)arg) return NULL;
	}
	return NULL;
}


void* managerThread(void* pArg) {
	uint threadCount = *(uint*) pArg;
	pthread_t pThreadPool[threadCount];

	for (int i = 0; i < threadCount; ++i) {
		int neverEnd = 0;
		pthread_create(pThreadPool[i], NULL, workerThread, (void*)&neverEnd);
	}

	sem_wait(&end);


}

void pushJob(Future* future) {
	pthread_mutex_lock(&mJobQueue);
	arrayPush(pJobQueue) = future;
	pthread_mutex_unlock(&mJobQueue);
}

Future* popJob() {
	pthread_mutex_lock(&mJobQueue);
	if (arrayLen(pJobQueue) == 0) return NULL;
	Future* ret = arrayTop(pJobQueue);
	arrayPop(pJobQueue);
	pthread_mutex_unlock(&mJobQueue);
	return ret;
}

void pushDone(Future* future) {
	pthread_mutex_lock(&mJobDone);
	arrayPush(pJobDone) = future;
	pthread_mutex_unlock(&mJobDone);
}

int searchDone(Future* future) {
	pthread_mutex_lock(&mJobDone);
	for (int i = 0; i < arrayLen(pJobDone); ++i) {
		if (pJobDone[i]->id == future->id) {
			pthread_mutex_unlock(&mJobDone);
			return 1;
		}
	}
	pthread_mutex_unlock(&mJobDone);
	return 0;
}

int countJobs() {
	pthread_mutex_lock(&mJobQueue);
	int ret = arrayLen(pJobQueue);
	pthread_mutex_unlock(&mJobQueue);
	return ret;
}

// void pushThread(pthread_t pThread) {
// 	pthread_mutex_lock(&mThreads);
// 	arrayPush(pThreads) = pThread;
// 	pthread_mutex_unlock(&mThreads);
// }

// pthread_t popThread() {
// 	pthread_mutex_lock(&mThreads);
// 	if (arrayLen(pThreads) == 0) return 0;
// 	pthread_t ret = arrayTop(pThreads);
// 	arrayPop(pThreads);
// 	pthread_mutex_unlock(&mThreads);
// 	return ret;
// }

// int countThreads() {
// 	pthread_mutex_lock(&mThreads);
// 	int ret = arrayLen(pThreads);
// 	pthread_mutex_unlock(&mThreads);
// 	return ret;
// }

