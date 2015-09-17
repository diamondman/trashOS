#include "syscall.h"
#include "core.h"
#include <libopencmsis/core_cm3.h>

unsigned int __attribute__ (( naked )) __get_PSP(void){
  __asm volatile(
		 "mrs r0, psp\n"
		 "bx lr\n"
		 );
}
void __attribute__ (( naked )) __set_PSP(unsigned int topOfProcStack){
  __asm volatile(
		 "msr psp, r0\n"
		 "bx lr\n"
		 );
}
unsigned int __attribute__ (( naked )) __get_MSP(void){
  __asm volatile(
		 "mrs r0, msp\n"
		 "bx lr\n"
		 );
}
void __attribute__ (( naked )) __set_MSP(unsigned int topOfProcStack){
  __asm volatile(
		 "msr msp, r0\n"
		 "bx lr\n"
		 );
}
unsigned int __attribute__ (( naked )) __get_CONTROL(void){
  __asm volatile(
		 "mrs r0, control\n"
		 "bx lr\n"
		 );
}
void __attribute__ (( naked )) sv_goto_process(void){
  __asm volatile(
		 "ORR lr, lr, #0x4\t\n"
		 "BX lr\t\n"
		 );
}

void osSwitchThreadStack(unsigned int oldstack, unsigned int newstack){
//__asm volatile(
//		 "MRS r12, PSP\n"
//		 "STMDB r12!, {r4-r11, LR}\n"
//		 "LDR r0, =OldPSPValue\n"
//		 "STR r12, [r0]\n"
//		 "LDR r0, =NewPSPValue\n"
//		 "LDR r12, [r0]\n"
//		 "LDMIA r12!, {r4-r11, LR}\n"
//		 "MSR PSP, r12\n"
//		 "BX lr\n"
//		 );
  __set_PSP(newstack);
  sv_goto_process();  
}
//void inline __start_critical(void){
//  __asm (
//	 "CPSID i\n"
//	 );
//}
//void inline __end_critical(void){
//  __asm (
//	 "CPSIE i\n"
//	 );
//}

