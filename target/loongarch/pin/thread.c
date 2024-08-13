#include "thread.h"
#include "pin_state.h"
#include "qemu.h"
#include "qemu/osdep.h"
#include <pthread.h>
#include <stdio.h>

uint32_t pin_thread_id_count = 0;
__thread uint32_t pin_thread_id = 0;

// NOTE:
// https://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_key_create.html
static void make_key(pthread_key_t *key) {
  assert(pthread_key_create(key, &PIN_thread_destructor) == 0);
}

TLS_KEY PIN_thread_create_key(void) {
  pthread_key_t key;
  fprintf(stderr, "thread create!\n");
  pin_thread_id = ++pin_thread_id_count;
  make_key(&key);
  return key;
}

void PIN_thread_bind_key(pthread_key_t key, const void *ptr) {
  if (pthread_getspecific(key) == NULL) {
    pthread_setspecific(key, ptr);
    ptr = pthread_getspecific(key);
  }
}

void *PIN_thread_getbind(pthread_key_t key) {
  void *ptr = NULL;
  ptr = pthread_getspecific(key);
  return ptr;
}

void PIN_thread_destructor(void *ptr) {
  fprintf(stderr, "finished!\n");
  if (PIN_state.thread_finish_cb != NULL) {
    PIN_state.thread_finish_cb(PIN_ThreadId(), NULL, 0, NULL);
  }
  free(ptr);
}

THREADID PIN_ThreadId(void) {
  return ((TaskState *)current_cpu->opaque)->ts_tid;
}

PIN_LOCK pin_lock_default(void)
{
    PIN_LOCK lock = {PTHREAD_MUTEX_INITIALIZER, 0};
    return lock;
}

VOID pin_get_lock(PIN_LOCK *lock)
{
    pthread_mutex_lock(&lock->_lock);
}

VOID pin_release_lock(PIN_LOCK *lock)
{
    pthread_mutex_unlock(&lock->_lock);
}
