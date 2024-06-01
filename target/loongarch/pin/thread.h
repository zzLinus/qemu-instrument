#ifndef PIN_THREAD_H
#define PIN_THREAD_H

#include "types.h"
#include <assert.h>
#include <pthread.h>

typedef UINT32 THREADID;

#ifdef __cplusplus
extern "C" {
#endif
THREADID PIN_ThreadId(void);
#ifdef __cplusplus
}
#endif

extern pthread_key_t key;
extern pthread_once_t key_once;


/* 仅内部使用 */
void PIN_thread_create(void);
void PIN_thread_destructor(void* ptr);


#endif
