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
		 "mrs r0, psp\t\n"      //Read psp for modification
		 "LDMIA r0!, {r4-r11}\n"//pop off higher registers
		 "msr psp, r0\t\n"      //Write back the psp
		 "ORR lr, lr, #0x4\t\n" //Set Process Flag
		 "BX lr\t\n"            //Return to process thread
		 );
}

void osSwitchThreadStack(unsigned int oldstack, unsigned int newstack){
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

