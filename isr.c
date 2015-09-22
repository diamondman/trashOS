#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>

#include <libopencm3/stm32/gpio.h>

#include "syscall.h"
#include "core.h"
#include "isr.h"
#include "config.h"

extern Thread TCB[];
extern int TCB_current_index;
extern unsigned short ticknum;
extern bool tickoverflow;

void  thread_changer(void){
  int last_thread = TCB_current_index;
  int index = TCB_current_index;
  for(int i = 0; i < TCB_LEN; i++){
    index = (index+1)%TCB_LEN;
    ThreadStatus* ts = &TCB[index].status;
    if(ts->runmode == THREAD_RUN) break;
    if(ts->runmode == THREAD_SLEEP &&
       ticknum >= ts->tick &&
       tickoverflow == ts->tickoverflow){
      ts->runmode = THREAD_RUN;
      break;
    }
  }
  TCB_current_index = index;

  if(last_thread == -1) goto switchit;

  Thread *last_t = &TCB[last_thread];
  last_t->stack = (unsigned int*)__get_PSP();

  //Check stack overflow
  if(*(last_t->stackTop) != 0xDEADBEEF &&
     last_t->status.runmode != THREAD_DISABLE){
    iprintf("\r\nOH SHIT! Stack WRONG MAGIC\r\n"
	    "THREAD ID: %d\r\n"
	    "STACKTOP:  %p\r\n"
	    "STACK:     %p\r\n",
	    last_thread, last_t->stackTop, last_t->stack);
    while(1){}
  }


switchit:
  ticknum++;
  if(ticknum==0) tickoverflow ^= 1;
  if(TCB[TCB_current_index].status.runmode != THREAD_RUN){
    usart_send_blocking(USART1,'!');
    sv_goto_idle();
  }
  osSwitchThreadStack((unsigned int)TCB[TCB_current_index].stack);
}


void __attribute__ (( naked )) sys_tick_handler(void){
  __asm__ volatile(
		   "tst lr, #4\t\n"          //Check EXC_RETURN[2] */
		   "ittt ne\t\n"             //If not equal THEN THEN THEN
		   "mrsne r12, psp\t\n"       //THEN r0=process stack pointer
		   "STMDBne r12!, {r4-r11}\n" //Back up registers to stack
		   "msrne psp, r12\t\n"       //Save new stack position from r0
		   "b %[thread_changer]\t\n" //Branch to main handler
		   : /* no output */
		   : [thread_changer] "i" (thread_changer) /* input */
		   : "r0" /* clobber */
		   );
}



void prvGetRegistersFromStack( uint32_t *);
void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
  /* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
  //osSchedulerEnabled = false;

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
  iprintf("HARDFAULT:\r\n"
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
  svc_number = ((char *)svc_args[6])[-2];

  switch(svc_number)
    {
    case SVC_WRITE_DATA:
      /* Handle SVC write data */
      sv_call_write_data_handler((char *)svc_args[0],
				 (int)svc_args[1]);
      break;
    case SVC_START_THREAD:
      sv_call_start_thread_handler((thread_func*)svc_args[0],
				   svc_args[1]);
      break;
    case SVC_END_CURRENT_THREAD:
      sv_call_end_current_thread_handler();
      break;
    case SVC_SLEEP_CURRENT_THREAD:
      sv_call_sleep_current_thread_handler((unsigned short)svc_args[0]);
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
  __asm__ volatile(
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
