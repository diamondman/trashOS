#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/systick.h>
#include <libopencmsis/core_cm3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "oled.h"
#include "syscall.h"

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

//extern bool osSchedulerEnabled;

void my_thread(void);
void my_thread2(void);

static void clock_setup(void){
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

static void gpio_setup(void){
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_PULL_UPDOWN, GPIO5);
}

static void usart_setup(void){
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_AFIO);
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_USART2);

  //USART 1
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

  usart_set_baudrate(USART1, 230400);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  //USART_CR1(USART1) |= USART_CR1_RXNEIE;

  usart_enable(USART1);

  //USART 2
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, GPIO_USART2_RX);
  
  usart_set_baudrate(USART2, 9600);
  usart_set_databits(USART2, 8);
  usart_set_stopbits(USART2, USART_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX_RX);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
  
  //USART_CR1(USART2) |= USART_CR1_RXNEIE;
  
  usart_enable(USART2);
}

static void spi_setup(void){
  //OLED
  //RST  PC4
  //D/C  PA1 (1 = Data)
  //NSS  PA4
  //SCK  PA5
  //MISO PA6 (IN)
  //MOSI PA7
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_AFIO);
  rcc_periph_clock_enable(RCC_SPI1);

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4 | GPIO5 | GPIO7);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, GPIO6);

  spi_reset(SPI1);

  spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

  spi_enable_software_slave_management(SPI1);
  spi_set_nss_high(SPI1);

  spi_enable(SPI1);


  //FONT TABLE
  //PB12 NSS   CS2  21
  //PB13 SCK   SCLK 20
  //PB14 MISO  SO   22
  //PB15 MOSI  SI   19
  rcc_periph_clock_enable(RCC_SPI2);
  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13 | GPIO15);
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, GPIO14);
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_PULL_UPDOWN, GPIO12);


  spi_reset(SPI2);

  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_32, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

  spi_enable_software_slave_management(SPI2);
  spi_set_nss_high(SPI2);
  gpio_set(GPIOB, GPIO12);

  spi_enable(SPI2);

}

static void tim_setup(void){
  rcc_periph_clock_enable(RCC_TIM1);
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_AFIO);

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		GPIO_TIM1_CH1); //GPIOA 8

  timer_reset(TIM1);

  timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT,
		 TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  timer_set_prescaler(TIM1, 0);
  timer_set_repetition_counter(TIM1, 0);
  timer_enable_preload(TIM1);
  timer_continuous_mode(TIM1);
  timer_set_period(TIM1, 72000000 / 32000);

  //timer_set_deadtime(TIM1, 10);
  //timer_set_enabled_off_state_in_idle_mode(TIM1);
  timer_set_disabled_off_state_in_run_mode(TIM1);
  timer_set_disabled_off_state_in_idle_mode(TIM1);
  timer_disable_break(TIM1);
  //timer_set_break_polarity_high(TIM1);
  timer_disable_break_automatic_output(TIM1);
  timer_set_break_lock(TIM1, TIM_BDTR_LOCK_OFF);

  //OUTPUT CONFIG
  timer_disable_oc_output(TIM1, TIM_OC1);
  timer_disable_oc_output(TIM1, TIM_OC1N);
  //timer_disable_oc_output(TIM1, TIM_OC2);
  //timer_disable_oc_output(TIM1, TIM_OC2N);
  //timer_disable_oc_output(TIM1, TIM_OC3);
  //timer_disable_oc_output(TIM1, TIM_OC3N);


  timer_disable_oc_clear(TIM1, TIM_OC1);
  timer_enable_oc_preload(TIM1, TIM_OC1);
  timer_set_oc_slow_mode(TIM1, TIM_OC1); //Fast mode just shaves off 3 cycles
  timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);

  /* Configure OC1. */
  timer_set_oc_polarity_high(TIM1, TIM_OC1);
  timer_set_oc_idle_state_set(TIM1, TIM_OC1);//don't quite get this one

  /* Set the capture compare value for OC1. */
  timer_set_oc_value(TIM1, TIM_OC1, 2250);

  timer_enable_oc_output(TIM1, TIM_OC1);
  //timer_enable_oc_output(TIM1, TIM_OC1N);

  timer_enable_preload(TIM1);

  timer_enable_break_main_output(TIM1);

  timer_enable_counter(TIM1);
}

static void sys_tick_setup(void) {
  /* 72MHz / 8 => 9000000 counts per second */
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

  /* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
  /* SysTick interrupt every N clock pulses: set reload to N-1 */
  systick_set_reload(100000+8999);

  //systick_interrupt_enable();

  /* Start counting. */
  systick_counter_enable();
}

uint16_t theta = 270;

int main(void){
  clock_setup();
  gpio_setup();
  usart_setup();
  iprintf("HI\r\n");
  tim_setup();
  spi_setup();
  oled_setup();
  sys_tick_setup();

  iprintf("\r\n\r\n*******\r\n");

  iprintf("my_thread: %p\r\n", my_thread);
  sv_call_start_thread(my_thread);
  sv_call_start_thread(my_thread2);
  //__disable_irq();

  for(int i = 0; i<25; i++)__asm__("NOP");
  oled_data_mode();

  for(int j = 0;j<5;j++)
    spi_send(SPI1, 0x00);

  for(int j = 0;j<64;j++)
    spi_send(SPI1, 0x00);

  for(int j = 0;j<50;j++)
    spi_send(SPI1, 0x00);

  timer_set_oc_value(TIM1, TIM_OC1, (sin(theta*(M_PI/180.0))+1)/2*(2250) );

  oled_cmd_mode();
  Set_Page(0);
  Set_Column(0);
  //iprintf("Space is limited \r\n");
  //iprintf("in a haiku\r\n");
  //iprintf("so it's hard to \r\n");
  //iprintf("finish what you\r\n");
  //
  //Set_Page(0);
  //Set_Column(0);
  //iprintf("***************\r\n");

  //sv_call_write_data("Hello World!", 12);

  //osSchedulerEnabled = true;
  systick_interrupt_enable();
  while(1){
    //iprintf("BB\r\n");
    //__WFI();
    //for(int i = 0; i<200000; i++)__asm__("NOP");
    //
    ////if(gpio_get(GPIOC, GPIO5)){
    //timer_set_oc_value(TIM1, TIM_OC1, (sin(theta*(M_PI/180.0))+1)/2*(2250) );
    //theta = (theta + 5)%360;
    //  //}
  }
}

void my_thread(void){
  iprintf("\r\nThread1 Started\r\n");
  //int* i1 = (int*)malloc(sizeof(int));
  //int* i2 = (int*)malloc(sizeof(int));
  //*i1 = 7;
  //*i2 = 9;
  //iprintf("Malloc: %p=%d; %p=%d\r\n", i1, *i1, i2, *i2);
  //free(i1);
  //free(i2);
  int abc=0;
  while(1){
    //iprintf("T1 %d\r\n", abc);
    oled_write_char('h');
    oled_write_char('i');
    usart_send_blocking(USART2, 'H');
    usart_send_blocking(USART2, 'I');
    usart_send_blocking(USART2, '\r');
    usart_send_blocking(USART2, '\n');
    iprintf("T1 %d\r\n", abc);
    abc++;


    for(int i = 0; i<20000000; i++)__asm__("NOP");
    // return;
  }
}

void my_thread2(void){
  iprintf("\r\nThread2 Started\r\n");
  while(1){
    for(int i = 0; i<200000; i++)__asm__("NOP");
    
    //if(gpio_get(GPIOC, GPIO5)){
    timer_set_oc_value(TIM1, TIM_OC1, (sin(theta*(M_PI/180.0))+1)/2*(2250) );
    theta = (theta + 5)%360;
    //}

    //iprintf("Hi Ima Thread\r\n");
    //for(int i = 0; i<20000000; i++)__asm__("NOP");
  }
}


//    unsigned int mode;
//    __asm volatile(
//		   "MRS %[mode], CONTROL\t\n"
//		   :[mode] "=X" (mode));
//    if(mode & 0x2)
//      iprintf("PSP\r\n");
//    else
//      iprintf("MSP\r\n");
