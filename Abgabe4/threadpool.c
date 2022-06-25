#include "threadpool.h"

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include "array.h"

static Future** pJobQueue;
static pthread_mutex_t mJobQueue;

static Future** pJobDone;
static pthread_mutex_t mJobDone;

// static pthread_t* pThreads;
// static pthread_mutex_t mThreads;

static sem_t end;

void serialize(Future* fut);

void* workerThread(void* arg);
void* managerThread(void* pArg);

void pushJob(Future* future);
Future* popJob();
int countJobs();

void pushDone(Future* future);
int searchnDeleteDone(Future* future);

static int numThreads;

//------ tpInit ------
int tpInit(size_t size) {
	numThreads = size;
	sem_init(&end, 0, 0);

	pthread_mutex_init(&mJobQueue, NULL);
	arrayInit(pJobQueue);
	arrayInit(pJobDone);

	pthread_t m;
	pthread_create(&m, NULL, managerThread, &numThreads);

	return 0;
}

//------ tpRelease ------
void tpRelease(void) {
	sem_post(&end);
	
	arrayRelease(pJobDone);
	arrayRelease(pJobQueue);
	//arrayRelease(pThreads);

	sem_destroy(&end);
}

//------ tpAsync ------
void tpAsync(Future* pFuture) {
	pushJob(pFuture);
}

//------ tpAwait ------
void tpAwait(Future* pFuture) {
	while (1) {
		if (searchnDeleteDone(pFuture)){
			return;
		} 
		else {
			int onlyOneJob = 1;
			workerThread((void*)&onlyOneJob);
		}
	}
}

//------ serialize ------
void serialize(Future* fut) {
	static atomic_int counter;
	++counter;
	fut->id = counter;
}

//------ workerThread ------
void* workerThread(void* arg) {
	Future* job;
	while (1) {
		job = popJob();
		if (job) {
			job->fn(job);
			pushDone(job);
		}
		else {
			usleep(1);
		}
		if (*(int*)arg) {
			return NULL;
		}
		//pthread_testcancel();
	}
	return NULL;
}

//------ managerThread ------
void* managerThread(void* pArg) {
	pthread_t pThreadPool[numThreads];
	for (int i = 0; i < numThreads; ++i) {
		int neverEnd = 0;
		pthread_create(&pThreadPool[i], NULL, workerThread, (void*)&neverEnd);
	}

	sem_wait(&end);
	for (int i = 0; i < numThreads; ++i) {
		pthread_cancel(pThreadPool[i]);
		pthread_join(pThreadPool[i], NULL);
	}
}

//------ pushJob ------
void pushJob(Future* pJob) {
	serialize(pJob);
	pthread_mutex_lock(&mJobQueue);
	arrayPush(pJobQueue) = pJob;
	pthread_mutex_unlock(&mJobQueue);
}

//------ popJob ------
Future* popJob() {
	pthread_mutex_lock(&mJobQueue);
	if (arrayLen(pJobQueue) == 0) return NULL;
	Future* ret = arrayPop(pJobQueue);
	pthread_mutex_unlock(&mJobQueue);
	return ret;
}

//------ pushDone ------
void pushDone(Future* future) {
	pthread_mutex_lock(&mJobDone);
	arrayPush(pJobDone) = future;
	pthread_mutex_unlock(&mJobDone);
}

//------ searchnDeleteDone ------
int searchnDeleteDone(Future* future) {
	pthread_mutex_lock(&mJobDone);
	for (int i = 0; i < arrayLen(pJobDone); ++i) {
		if (pJobDone[i]->id == future->id) {
			//swap elements
			Future* temp = pJobDone[arrayLen(pJobDone)-1];
			pJobDone[arrayLen(pJobDone)-1] = pJobDone[i];
			pJobDone[i] = temp;
			arrayPop(pJobDone);

			pthread_mutex_unlock(&mJobDone);
			return 1;
		}
	}

	pthread_mutex_unlock(&mJobDone);
	return 0;
}

//------ countJobs ------
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

