#include <libopencmsis/core_cm3.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oled.h"
#include "syscall.h"
#include "core.h"
#include "isr.h"
#include "config.h"

/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) __asm volatile ("svc %[immediate]"::[immediate] "I" (code))

Thread TCB[TCB_LEN];
int volatile TCB_current_index = -1;
unsigned int ticknum = 0;
bool tickoverflow = false;

/*
 * Use a normal C function, the compiler will make sure that this is going
 * to be called using the standard C ABI which ends in a correct stack
 * frame for our SVC call
 */
__attribute__ ((noinline)) void sv_call_write_data(char *string, int length){
  svc(SVC_WRITE_DATA);
}
__attribute__ ((noinline)) void sv_call_start_thread(thread_func func, unsigned int stack_size){
  svc(SVC_START_THREAD);
}
__attribute__ ((noinline)) void sv_call_end_current_thread(){
  svc(SVC_END_CURRENT_THREAD);
}
__attribute__ ((noinline)) void sv_call_sleep_current_thread(unsigned int ticks){
  svc(SVC_SLEEP_CURRENT_THREAD);
}

void sv_call_write_data_handler(char *string, int length){
  iprintf("Supervisor call \"%s\", length %d.\r\n", string, length);
}

static Thread* osCreateThread(thread_func func, unsigned int stack_base,
			 unsigned int stack_size){
  int tcb_i; 
  for(tcb_i = 0; tcb_i<TCB_LEN; tcb_i++)
    if(TCB[tcb_i].status.runmode == THREAD_DISABLE) break;

  if(tcb_i==TCB_LEN) return NULL;

  unsigned int* stack = (unsigned int*)stack_base;
  if(stack==0){
    if(stack_size<(8*sizeof(int))) return NULL;
    stack = (unsigned int*)malloc(stack_size);
  }
  *stack = 0xDEADBEEF;

  Thread* t = &TCB[tcb_i];
  t->status.runmode = THREAD_RUN;
  t->stackTop = stack;
  t->stack = stack+(stack_size-16);

  t->stack[5+8] = (unsigned int)sv_call_end_current_thread;
  t->stack[6+8] = (unsigned int)func;
  t->stack[7+8] = 0x01000000;
  
  return t;
}

void sv_call_start_thread_handler(thread_func func, unsigned int stack_size){
  if(osCreateThread(func, 0,
		    (stack_size?stack_size:
		     (STACK_WORD_SIZE+1)*sizeof(int))
		    )==NULL)
    iprintf(" Create Failed\r\n");
}

void sv_call_end_current_thread_handler(void){
  __disable_irq();
  {
    Thread* t = &TCB[TCB_current_index];
    t->status.runmode = THREAD_DISABLE;
    free(t->stackTop);
  }
  __enable_irq();

  thread_changer();
}

void sv_call_sleep_current_thread_handler_main(unsigned short);
void __attribute__ (( naked )) sv_call_sleep_current_thread_handler(unsigned short ticks){
  __asm__ volatile(
		   "tst lr, #4\t\n"          //Check EXC_RETURN[2] */
		   "ittt ne\t\n"             //If not equal THEN THEN THEN 
		   "mrsne r12, psp\t\n"       //THEN r0=process stack pointer
		   "STMDBne r12!, {r4-r11}\n" //Back up registers to stack
		   "msrne psp, r12\t\n"       //Save new stack position from r0
		   "b %[func]\t\n" //Branch to main handler
		   : /* no output */
		   : [func] "i" (sv_call_sleep_current_thread_handler_main) /* input */
		   : "r12" /* clobber */
		   );
}

void sv_call_sleep_current_thread_handler_main(unsigned short ticks){
  __disable_irq();
  {
    ThreadStatus* ts = &TCB[TCB_current_index].status;
    ts->runmode = THREAD_SLEEP;
    ts->tick = ticks+(unsigned short)ticknum;
    ts->tickoverflow = (ts->tick>ticknum)?tickoverflow:~tickoverflow;
  }
  __enable_irq();
  
  thread_changer();
}
