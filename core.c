#include "core.h"

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
