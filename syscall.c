#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <reent.h>
#include <sys/unistd.h>
#include <_syslist.h>

#include "oled.h"
#include "syscall.h"
#include "core.h"

//#include "lcd.h"

/*#include <_ansi.h>
#include <_syslist.h>

#include <reent.h>
#include <unistd.h>
#include <_syslist.h>*/
static void sv_goto_process(void);




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
      oled_write_char(c);
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
  //extern char heap_low; /* Defined by the linker */
  //extern char heap_top; /* Defined by the linker */
  char *prev_heap_end;
  //
  if (heap_end == 0) {
    heap_end = &end;//heap_low;
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
 * SVC sample for GCC-Toolchain on Cortex-M3/M4 or M4F
 */

/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) __asm volatile ("svc %[immediate]"::[immediate] "I" (code))

#define SVC_WRITE_DATA 1
#define SVC_START_THREAD 2
/*
 * Handler function definition, same parameters as the SVC function below
 */
void sv_call_write_data_handler(char *string, int length);
void sv_call_start_thread_handler(unsigned int *svc_args, thread_func* func);


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

/*
 * SVC handler
 * In this function svc_args points to the stack frame of the SVC caller
 * function. Up to four 32-Bit sized arguments can be mapped easily:
 * The first argument (r0) is in svc_args[0],
 * The second argument (r1) in svc_args[1] and so on..
 */
void sv_call_handler_main(unsigned int *svc_args)
{
  unsigned int svc_number;

  /*
   * We can extract the SVC number from the SVC instruction. svc_args[6]
   * points to the program counter (the code executed just before the svc
   * call). We need to add an offset of -2 to get to the upper byte of
   * the SVC instruction (the immediate value).
   */
  printf("CONTROL: %x\r\n", __get_CONTROL());

  printf("NUM: %p\r\n", svc_args);
  printf("xPSR: %u\r\n", svc_args[7]);
  svc_number = ((char *)svc_args[6])[-2];

  switch(svc_number)
    {
    case SVC_WRITE_DATA:
      /* Handle SVC write data */
      sv_call_write_data_handler((char *)svc_args[0],
				 (int)svc_args[1]);
      break;
    case SVC_START_THREAD:
      sv_call_start_thread_handler(svc_args, (thread_func*)svc_args[0]);
      break;
    default:
      /* Unknown SVC */
      break;
    }
}

/*
 * SVC Handler entry, put a pointer to this function into the vector table
 */
void __attribute__ (( naked )) sv_call_handler(void)
{
  /*
   * Get the pointer to the stack frame which was saved before the SVC
   * call and use it as first parameter for the C-function (r0)
   * All relevant registers (r0 to r3, r12 (scratch register), r14 or lr
   * (link register), r15 or pc (programm counter) and xPSR (program
   * status register) are saved by hardware.
   */
  __asm volatile(
		 "tst lr, #4\t\n" /* Check EXC_RETURN[2] */
		 "ite eq\t\n"
		 "mrseq r0, msp\t\n"
		 "mrsne r0, psp\t\n"
		 "b %[sv_call_handler_main]\t\n"
		 : /* no output */
		 : [sv_call_handler_main] "i" (sv_call_handler_main) /* input */
		 : "r0" /* clobber */
		 );
}

static void __attribute__ (( naked )) sv_goto_process(void){
  __asm volatile(
		 "ORR lr, lr, #0x4\t\n"
		 "BX lr\t\n"
		 );
}

void sv_call_write_data_handler(char *string, int length){
  printf("Supervisor call \"%s\", length %d.\r\n", string, length);
}
void my_thread(void);
void __thread_exit(void);
void __thread_exit(void){
  printf("Thread Terminated\r\n");
  for(;;){}
}

void sv_call_start_thread_handler(unsigned int *orig_stack, void (*func)(void)){
  //unsigned int* stack2 = (unsigned int*)(0x20002800);

  __set_PSP(0x20004000);//0x20005000-(14*sizeof(int)));//(unsigned int)orig_stack);
  unsigned int* pstack = (unsigned int*)__get_PSP();
  unsigned int* mstack = (unsigned int*)__get_MSP();

  printf("MStack: %p\r\n", mstack);
  printf("PStack: %p\r\n", pstack);

  pstack[5] = (unsigned int)__thread_exit;
  pstack[6] = (unsigned int)func;
  printf("xPSR val: %8x\r\n", pstack[7]);
  pstack[7] = 0x01000000;

  printf("func: %p\r\n", func);

  sv_goto_process();

}




void prvGetRegistersFromStack( uint32_t *);
void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
  /* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
  volatile unsigned int r0;
  volatile unsigned int r1;
  volatile unsigned int r2;
  volatile unsigned int r3;
  volatile unsigned int r12;
  volatile unsigned int lr; /* Link register. */
  volatile unsigned int pc; /* Program counter. */
  volatile unsigned int psr;/* Program status register. */
  
  r0 = pulFaultStackAddress[ 0 ];
  r1 = pulFaultStackAddress[ 1 ];
  r2 = pulFaultStackAddress[ 2 ];
  r3 = pulFaultStackAddress[ 3 ];

  r12 = pulFaultStackAddress[ 4 ];
  lr = pulFaultStackAddress[ 5 ];
  pc = pulFaultStackAddress[ 6 ];
  psr = pulFaultStackAddress[ 7 ];

  /* When the following line is hit, the variables contain the register values. */
  printf("HARDFAULT:\r\n"
	 "\tR13: %p\r\n"
	 "\tR0:  0x%8x\r\n"
	 "\tR1:  0x%8x\r\n"
	 "\tR2:  0x%8x\r\n"
	 "\tR3:  0x%8x\r\n"
	 "\tR12: 0x%8x\r\n"
	 "\tLR:  0x%8x\r\n"
	 "\tPC:  0x%8x\r\n"
	 "\tPSR: 0x%8x \r\n",
	 pulFaultStackAddress, r0, r1, r2, r3, r12, lr, pc, psr);

  for( ;; );
}


//static void hard_fault_handler( void ) __attribute__( ( naked ) );
void __attribute__( ( naked ) ) hard_fault_handler(void)
{
      __asm volatile
	(
	         " tst lr, #4                                \n"
		 " ite eq                                    \n"
		 " mrseq r0, msp                             \n"
		 " mrsne r0, psp                             \n"
		 " ldr r1, [r0, #24]                         \n"
		 " ldr r2, handler2_address_const            \n"
		 " bx r2                                     \n"
		 " handler2_address_const: .word prvGetRegistersFromStack\n"
	 );
}

void mem_manage_handler(void){
  for(;;);
}
