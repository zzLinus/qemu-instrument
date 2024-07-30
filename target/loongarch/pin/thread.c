#include "thread.h"
#include "qemu/osdep.h"
#include "qemu.h"
#include "pin_state.h"
#include <stdio.h>

uint32_t pin_thread_id_count = 0;
__thread uint32_t pin_thread_id = 0;


// NOTE: https://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_key_create.html
static void make_key(pthread_key_t* key)
{
    assert(pthread_key_create(key, &PIN_thread_destructor) == 0);
}

TLS_KEY PIN_thread_create_key(void)
{
    pthread_key_t key;
    fprintf(stderr,"thread create!\n");
    void* ptr;
    make_key(&key);
    if((ptr = pthread_getspecific(key)) == NULL){
        ptr = malloc(PIN_buffer_info.buffer_size[0]);
        pthread_setspecific(key,ptr);
    }
    pin_thread_id = ++pin_thread_id_count;
    return key;
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
