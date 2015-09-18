#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/unistd.h>

#include "oled.h"
#include "syscall.h"
#include "core.h"

/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) __asm volatile ("svc %[immediate]"::[immediate] "I" (code))

Thread TCB[TCB_LEN];
int volatile TCB_current_index = -1;
bool volatile osSchedulerEnabled;

int _close(int file) { return -1; }

int _fstat(int file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file) { return 1; }

_off_t _lseek(int file, _off_t ptr, int dir) { return 0; }

//int _open(const char *name, int flags, int mode) { return -1; }

int _write(int file, const void * s , size_t len){
  int count = 0;
  char* s2 = (char*)s;
  if(file == 1 || file == 2){
    while (count<len){
      uint8_t c = (uint8_t) *s2++;
      usart_send_blocking(USART1, c);
      //usart_send_blocking(USART2, c);
      //oled_write_char(c);
      count++;
    }
  }

  return len;
}

int _read(int file, void *ptr, size_t len) {
  int num = 0;
/*int n;
  switch (file) {
  case STDIN_FILENO:
    for (n = 0; n < len; n++) {
      char c = Usart1Get();
      *ptr++ = c;
      num++;
    }
    break;
  default:
    errno = EBADF;
    return -1;
    }*/
  return num;
}

char *heap_end = 0;
void * _sbrk(ptrdiff_t incr) {
  extern char end;
  char *prev_heap_end;
  if (heap_end == 0) {
    heap_end = &end;
  }
  prev_heap_end = heap_end;
  
  //if (heap_end + incr > &heap_top) {
  ///* Heap and stack collision */
  //  return (caddr_t)0;
  //}
  //
  heap_end += incr;
  return (caddr_t) prev_heap_end;
  //return (caddr_t) 0;
}


/*
 * Use a normal C function, the compiler will make sure that this is going
 * to be called using the standard C ABI which ends in a correct stack
 * frame for our SVC call
 */
__attribute__ ((noinline)) void sv_call_write_data(char *string, int length)
{
  svc(SVC_WRITE_DATA);
}

__attribute__ ((noinline)) void sv_call_start_thread(void (*func)(void))
{
  svc(SVC_START_THREAD);
}


void sv_call_write_data_handler(char *string, int length){
  iprintf("Supervisor call \"%s\", length %d.\r\n", string, length);
}
void my_thread(void);
void __thread_exit(void);
void __thread_exit(void){
  iprintf("Thread Terminated\r\n");
  for(;;){}
}

static Thread* getEmptyTCBEntry(void){
  for(int i = 0; i<TCB_LEN; i++){
    if(TCB[i].status==0){
      iprintf("TCB: %u\r\n", i);
      return &TCB[i];
    }
  }
  return NULL;
}

static Thread* osCreateThread(void (*func)(void), unsigned int stack_base,
			 unsigned int stack_size){
  Thread* t = getEmptyTCBEntry();
  if(t==NULL)
    return NULL;

  unsigned int* stack = (unsigned int*)stack_base;
  if(stack==0){
    if(stack_size<(8*sizeof(int)))
      return NULL;
    stack = (unsigned int*)malloc(stack_size);
  }
  *stack = 0xDEADBEEF;
  iprintf("STACKALLOC: %p\r\n", stack);
  
  t->status = TCB_THREAD_INUSE;
  t->stackTop = stack;
  t->stackSize = stack_size;
  t->stack = stack+(stack_size-16);

  t->stack[5+8] = (unsigned int)__thread_exit;
  t->stack[6+8] = (unsigned int)func;
  t->stack[7+8] = 0x01000000;
  
  return t;
}

void sv_call_start_thread_handler(unsigned int *orig_stack, void (*func)(void)){
  Thread* t = osCreateThread(func, 0, (STACK_WORD_SIZE+1)*sizeof(int));
  if(t==NULL){
    iprintf("Thread Create Failed\r\n");
    return;
  }
}

