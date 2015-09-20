#include "config.h"

typedef void thread_func(void);

//void sv_call_handler(void);
void sv_call_handler_main(unsigned int*);

void sv_call_write_data(char*, int);
void sv_call_start_thread(thread_func*, unsigned int);
void sv_call_end_current_thread(void);
void sv_call_sleep_current_thread(unsigned int);
  
/*
 * Handler function definition, same parameters as the SVC function below
 */
void sv_call_write_data_handler(char*, int);
void sv_call_start_thread_handler(thread_func*, unsigned int);
void sv_call_end_current_thread_handler(void);
void sv_call_sleep_current_thread_handler(unsigned short);


#define SVC_WRITE_DATA 1
#define SVC_START_THREAD 2
#define SVC_END_CURRENT_THREAD 3
#define SVC_SLEEP_CURRENT_THREAD 4


/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) __asm volatile ("svc %[immediate]"::[immediate] "I" (code))

#define THREAD_DISABLE 0
#define THREAD_RUN 1
#define THREAD_SLEEP 2

typedef struct {
  unsigned int runmode : 2;
  unsigned short tick;
} ThreadStatus;

typedef struct {
  unsigned int *stack;
  unsigned int *stackTop;
  ThreadStatus status;
} Thread;

#define sleep(ticks) (sv_call_sleep_current_thread(ticks))
#define sleepms(ms) (sleep(ms*SYSTICKS_PER_MS))
