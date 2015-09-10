#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include "oled.h"
#include "config.h"

static uint8_t row_num, char_col_num = 0;

inline void Set_Display_On_Off(uint8_t x){
  //x = 0 means OFF
  spi_send(SPI1, 0xAE | (x & 1));
}

inline void Set_Multiplex_Ratio(uint8_t x){
  spi_send(SPI1, 0xA8);
  spi_send(SPI1, (x & 0x3F));
}

inline void Set_Inverse_Display(uint8_t x){
  //x = 0 means NORMAL
  spi_send(SPI1, 0xA6 | (x & 1));
}

inline void Set_Entire_Display(uint8_t x){
  //x = 0 means NORMAL
  spi_send(SPI1, 0xA4 | (x & 1));
}

inline void Set_Segment_Remap(uint8_t x){
  spi_send(SPI1, 0xA0 | (x & 1));
}

inline void Set_Precharge_Period(uint8_t x){
  spi_send(SPI1, 0xD9);
  spi_send(SPI1, x);
}

inline void Set_Start_Line(uint8_t x){
  spi_send(SPI1, 0x40 | (x & 0x3F)); //actually correct
}

inline void Set_Display_Offset(uint8_t x){
  spi_send(SPI1, 0xD3);
  spi_send(SPI1, (x & 0x3F));
}

inline void Set_Common_Remap(uint8_t x){
  //x = 0 or 8
  spi_send(SPI1, 0xC0 | (x & 0x08));
}

inline void Set_VCOMH(uint8_t x){
  spi_send(SPI1, 0xDB);
  spi_send(SPI1, (x & 0x3C));
}

inline void Set_Addressing_Mode(uint8_t x){
    spi_send(SPI1, 0x20);
    spi_send(SPI1, (x & 0x03));
}

inline void Set_Display_Clock(uint8_t x){
    spi_send(SPI1, 0xD5);
    spi_send(SPI1, x);
}

inline void Set_Contrast_Control(uint8_t x){
  spi_send(SPI1, 0x81);
  spi_send(SPI1, x);
}

inline void Set_Area_Brightness(uint8_t x){
  spi_send(SPI1, 0x82);
  spi_send(SPI1, x);
}

inline void Set_Area_Color(uint8_t x){
  spi_send(SPI1, 0xD8);
  spi_send(SPI1, (x & 0x35));
}

inline void Set_LUT(uint8_t x, uint8_t a, uint8_t b, uint8_t c){
  spi_send(SPI1, 0x91);
  spi_send(SPI1, (x & 0x3F));
  spi_send(SPI1, (a & 0x3F));
  spi_send(SPI1, (b & 0x3F));
  spi_send(SPI1, (c & 0x3F));
}

inline void Set_Master_Config(void){
  spi_send(SPI1, 0xAD);
  spi_send(SPI1, 0xAE);
}

inline void Set_Common_Config(uint8_t x){
  // Set Alternative Configuration (0x00/0x10)
  spi_send(SPI1, 0xDA);
  spi_send(SPI1, 0x02 | (x & 0x30));
}

inline void Set_ColumnAddress(uint8_t start_, uint8_t stop_){
  spi_send(SPI1, 0x21);
  spi_send(SPI1, start_);
  spi_send(SPI1, stop_);
}

inline void Set_PageAddress(uint8_t start_, uint8_t stop_){
  spi_send(SPI1, 0x22);
  spi_send(SPI1, (start_ & 0x07));
  spi_send(SPI1, (stop_ & 0x07));
}

inline void oled_cmd_mode(void){
    gpio_clear(GPIOA, GPIO1);//data mode
}

inline void oled_data_mode(void){
    gpio_set(GPIOA, GPIO1);//data mode
}

inline void Set_Page(uint8_t page){
  spi_send(SPI1, 0xB0 | (page&0x07));
  row_num = page%3;
}

inline void Set_Column(uint8_t col){
  char_col_num = 0; //SHIT SOLUTION;
  col = col+4;
  spi_send(SPI1, 0x00 | (col&0x0F));
  spi_send(SPI1, 0x10 | ((col>>4)&0x0F));
}

void oled_setup(void){
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO1);
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO4);

  oled_cmd_mode();
  gpio_clear(GPIOC, GPIO4);
  for(int i = 0; i< 500000;i++)__asm__("NOP");
  gpio_set(GPIOC, GPIO4);
  for(int i = 0; i< 500000;i++)__asm__("NOP");

  Set_Display_On_Off(0x00);         // Display Off (0x00/0x01)
  Set_Display_Clock(0x10);          // Set Clock as 160 Frames/Sec
  Set_Multiplex_Ratio(0x1F);        // 1/32 Duty (0x0F~0x3F)
  Set_Display_Offset(0x00);         // Shift Mapping RAM Counter (0x00~0x3F)
  Set_Start_Line(0x00);             // Set Mapping RAM Display Start Line (0x00~0x3F)
  Set_Master_Config();              // Disable Embedded DC/DC Converter (0x00/0x01)
  Set_Area_Color(0x05);             // Set Monochrome & Low Power Save Mode
  Set_Addressing_Mode(0x02);        // Set Page Addressing Mode (0x00/0x01/0x02)
  Set_Segment_Remap(0x01);          // Set SEG/Column Mapping (0x00/0x01)
  Set_Common_Remap(0x08);           // Set COM/Row Scan Direction (0x00/0x08)
  Set_Common_Config(0x10);          // Set Alternative Configuration (0x00/0x10)
  Set_LUT(0x3F,0x3F,0x3F,0x3F);     // Define All Banks Pulse Width as 64 Clocks
  Set_Contrast_Control(BRIGHTNESS); // Set SEG Output Current
  Set_Area_Brightness(BRIGHTNESS);  // Set Brightness for Area Color Banks
  Set_Precharge_Period(0xD2);       // Set Pre‐Charge as 13 Clocks & Discharge as 2 Clock
  Set_VCOMH(0x08);                  // Set VCOM Deselect Level
  Set_Entire_Display(0x00);         // Disable Entire Display On (0x00/0x01)
  Set_Inverse_Display(0x00);        // Disable Inverse Display On (0x00/0x01)

  for(int page = 0; page<4; page++){
    oled_cmd_mode();
    Set_Page(page);
    Set_Column(0);
    for(int i = 0; i<1000; i++)__asm__("NOP");
    oled_data_mode();
    for(int i = 0; i < 132; i++)
      spi_send(SPI1, 0x00);
    for(int i = 0; i<1000; i++)__asm__("NOP");
  }
  
  oled_cmd_mode();
  Set_Display_On_Off(0x01);         // Display On (0x00/0x01)
  Set_ColumnAddress(5, 131);
}

void oled_write_char(uint8_t c){
  if(c=='\r'){
      oled_cmd_mode();
      Set_Column(0);
      return;
  }
  if(c=='\n'){
    oled_cmd_mode();
    Set_Page(row_num+1);
    return;
  }
  if(c<0x20) return;
  char_col_num++;
  if(char_col_num >= 7*18){
      oled_cmd_mode();
      Set_Column(0);
      Set_Page(row_num+1);
  }
  
  oled_data_mode();
  uint32_t addr = ((c-0x20)*8);
    
  gpio_clear(GPIOB, GPIO12);
  //for(int i = 0; i<5000; i++)__asm__("NOP");
  spi_send(SPI2, 0x0B);
  spi_send(SPI2, (addr>>16)&0xFF);
  spi_send(SPI2, (addr>>8)&0xFF);
  spi_send(SPI2, (addr>>0)&0xFF);
  spi_send(SPI2, 0xFF);//dummy write
  
  for(int i = 0; i<350; i++)__asm__("NOP");
  
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  spi_send(SPI1, spi_xfer(SPI2, 0x00));
  //spi_send(SPI1, spi_xfer(SPI2, 0x00));
  
  gpio_set(GPIOB, GPIO12);
}
