#pragma once 
#include<assert.h>
#include<math.h>
#include<lua.hpp>



#ifdef __VERSION__
#include<arpa/inet.h>
#include<pthread.h>
#include<signal.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<errno.h>
#include<math.h>
#include<netinet/tcp.h> 
#define sleep1 timespec ts = {0, 1000000};nanosleep(&ts, 0)

#define lock_t int
#define INIT_LOCK(lock)  lock = 0
#define LOCK(lock) while (__sync_lock_test_and_set(&lock,1)) {}
#define UNLOCK(lock) __sync_lock_release(&lock)
#define DEBUG_PUTS(str) 


#else
#define sleep1 Sleep(1)

#define lock_t CRITICAL_SECTION
#define INIT_LOCK(lock) InitializeCriticalSection(&lock)
#define LOCK(lock)  EnterCriticalSection(&lock)
#define UNLOCK(lock) LeaveCriticalSection(&lock)

#define DEBUG_PUTS(str) puts(str)

#include<windows.h>
#include<sys/timeb.h>
#include<time.h>

#endif



inline double roundM(double r){
	return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}
