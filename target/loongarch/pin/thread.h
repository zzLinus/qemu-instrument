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

typedef struct
{
    pthread_mutex_t _lock;           ///< Implements the lock.
    INT32 _owner;                    ///< Used for debugging, typically the ID of the lock owner.
}PIN_LOCK;

// use internally
TLS_KEY PIN_thread_create_key(void);
VOID PIN_thread_destructor(void* ptr);
VOID PIN_thread_bind_key(pthread_key_t key, const void* ptr);
VOID* PIN_thread_getbind(pthread_key_t key);
PIN_LOCK pin_lock_default(void);
VOID pin_get_lock(PIN_LOCK* lock);
VOID pin_release_lock(PIN_LOCK* lock);

#endif
