#include "thread.h"
#include "qemu/osdep.h"
#include "qemu.h"
#include "pin_state.h"
#include <stdio.h>

uint32_t pin_thread_id_count = 0;
__thread uint32_t pin_thread_id = 0;

pthread_key_t key;
pthread_once_t key_once = PTHREAD_ONCE_INIT;

// NOTE: https://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_key_create.html
static void make_key(void)
{
    assert(pthread_key_create(&key, &PIN_thread_destructor) == 0);
}

void PIN_thread_create(void)
{
    fprintf(stderr,"thread create! 1\n");
    void* ptr;
    pthread_once(&key_once, make_key);
    if((ptr = pthread_getspecific(key)) == NULL){
        fprintf(stderr,"malloc buffer for key %d\n",key);
        ptr = malloc(PIN_buffer_info.buffer_size[0]);
        pthread_setspecific(key,ptr);
    }
    pin_thread_id = ++pin_thread_id_count;
}

void PIN_thread_destructor(void* ptr){
    fprintf(stderr,"finished!\n");
    if(PIN_state.thread_finish_cb != NULL)
    {
        PIN_state.thread_finish_cb(PIN_ThreadId(), NULL,0,NULL);
    }
    free(ptr);
}

THREADID PIN_ThreadId(void)
{
    return ((TaskState *)current_cpu->opaque)->ts_tid;
}
