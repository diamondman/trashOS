#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include <stdio.h>
#include "oled.h"
#include "config.h"

#define OLED_MODE_CMD 0
#define OLED_MODE_DAT 1
#define OLED_MODE_UNK 0xff

static uint8_t oledmode = OLED_MODE_UNK;
static uint8_t fontchip_enabled = true;
//The plus 2 is for \r\n
//#define OLED_BUFF_LEN (OLED_CHAR_ROWS*(OLED_CHAR_COLS+2))
static unsigned char oled_buff[OLED_CHAR_ROWS][OLED_CHAR_COLS+1];
static unsigned int oled_buff_row_start = 0;
static unsigned int oled_buff_row_end = 0;
static unsigned int oled_buff_col_cursor = 0;
static unsigned int oled_buff_active_rows = 1;

//static unsigned char* oled_lines[OLED_CHAR_ROWS] = {};
//static unsigned int oled_line_start = 0;
//static unsigned int oled_line_end = 0;

inline void Set_Display_On_Off(uint8_t x){
  //x = 0 means OFF
  oled_cmd_mode();
  spi_send(SPI1, 0xAE | (x & 1));
}

inline void Set_Multiplex_Ratio(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xA8);
  spi_send(SPI1, (x & 0x3F));
}

inline void Set_Inverse_Display(uint8_t x){
  //x = 0 means NORMAL
  oled_cmd_mode();
  spi_send(SPI1, 0xA6 | (x & 1));
}

inline void Set_Entire_Display(uint8_t x){
  //x = 0 means NORMAL
  oled_cmd_mode();
  spi_send(SPI1, 0xA4 | (x & 1));
}

inline void Set_Segment_Remap(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xA0 | (x & 1));
}

inline void Set_Precharge_Period(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xD9);
  spi_send(SPI1, x);
}

inline void Set_Start_Line(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0x40 | (x & 0x3F)); //actually correct
}

inline void Set_Display_Offset(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xD3);
  spi_send(SPI1, (x & 0x3F));
}

inline void Set_Common_Remap(uint8_t x){
  //x = 0 or 8
  oled_cmd_mode();
  spi_send(SPI1, 0xC0 | (x & 0x08));
}

inline void Set_VCOMH(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xDB);
  spi_send(SPI1, (x & 0x3C));
}

inline void Set_Addressing_Mode(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0x20);
  spi_send(SPI1, (x & 0x03));
}

inline void Set_Display_Clock(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xD5);
  spi_send(SPI1, x);
}

inline void Set_Contrast_Control(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0x81);
  spi_send(SPI1, x);
}

inline void Set_Area_Brightness(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0x82);
  spi_send(SPI1, x);
}

inline void Set_Area_Color(uint8_t x){
  oled_cmd_mode();
  spi_send(SPI1, 0xD8);
  spi_send(SPI1, (x & 0x35));
}

inline void Set_LUT(uint8_t x, uint8_t a, uint8_t b, uint8_t c){
  oled_cmd_mode();
  spi_send(SPI1, 0x91);
  spi_send(SPI1, (x & 0x3F));
  spi_send(SPI1, (a & 0x3F));
  spi_send(SPI1, (b & 0x3F));
  spi_send(SPI1, (c & 0x3F));
}

inline void Set_Master_Config(void){
  oled_cmd_mode();
  spi_send(SPI1, 0xAD);
  spi_send(SPI1, 0xAE);
}

inline void Set_Common_Config(uint8_t x){
  // Set Alternative Configuration (0x00/0x10)
  oled_cmd_mode();
  spi_send(SPI1, 0xDA);
  spi_send(SPI1, 0x02 | (x & 0x30));
}

inline void Set_ColumnAddress(uint8_t start_, uint8_t stop_){
  oled_cmd_mode();
  spi_send(SPI1, 0x21);
  spi_send(SPI1, start_);
  spi_send(SPI1, stop_);
}

inline void Set_PageAddress(uint8_t start_, uint8_t stop_){
  oled_cmd_mode();
  spi_send(SPI1, 0x22);
  spi_send(SPI1, (start_ & 0x07));
  spi_send(SPI1, (stop_ & 0x07));
}

inline void oled_cmd_mode(void){
  if(oledmode==OLED_MODE_CMD) return;
  oledmode = OLED_MODE_CMD;
  gpio_clear(GPIOA, GPIO1);
  for(int i = 0; i<350; i++)__asm__("NOP");
}

inline void oled_data_mode(void){
  if(oledmode==OLED_MODE_DAT) return;
  oledmode = OLED_MODE_DAT;
  gpio_set(GPIOA, GPIO1);
  for(int i = 0; i<350; i++)__asm__("NOP");
}

inline void Set_Page(uint8_t page){
  oled_cmd_mode();
  for(int i = 0; i<500; i++)__asm__("NOP");
  spi_send(SPI1, 0xB0 | (page&0x07));
  for(int i = 0; i<500; i++)__asm__("NOP");
}

inline void Set_Char_Column(uint8_t col){
  Set_Column(col*(OLED_CHAR_WIDTH+OLED_CHAR_X_PADDING));
}

inline void Set_Column(uint8_t col){
  oled_cmd_mode();
  for(int i = 0; i<500; i++)__asm__("NOP");
  //char_col_num = 0; //SHIT SOLUTION;
  col = col+OLED_X_DEADSPACE;
  spi_send(SPI1, 0x00 | (col&0x0F));
  spi_send(SPI1, 0x10 | ((col>>4)&0x0F));
  for(int i = 0; i<500; i++)__asm__("NOP");
}

//inline void

void oled_setup(void){
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO1);
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO4);

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
  Set_Contrast_Control(OLED_BRIGHTNESS); // Set SEG Output Current
  Set_Area_Brightness(OLED_BRIGHTNESS);  // Set Brightness for Area Color Banks
  Set_Precharge_Period(0xD2);       // Set Pre‐Charge as 13 Clocks & Discharge as 2 Clock
  Set_VCOMH(0x08);                  // Set VCOM Deselect Level
  Set_Entire_Display(0x00);         // Disable Entire Display On (0x00/0x01)
  Set_Inverse_Display(0x00);        // Disable Inverse Display On (0x00/0x01)

  for(int page = 0; page<4; page++){
    Set_Page(page);
    Set_Column(0);
    for(int i = 0; i<1000; i++)__asm__("NOP");
    oled_data_mode();
    for(int i = 0; i < OLED_PIXEL_WIDTH; i++)
      spi_send(SPI1, 0x00);
    for(int i = 0; i<1000; i++)__asm__("NOP");
  }

  Set_Display_On_Off(0x01);         // Display On (0x00/0x01)
  Set_ColumnAddress(5, 131);

  oled_buff_row_start = 0;
  oled_buff_row_end = 0;
  oled_buff_col_cursor = 0;
  for(int r=0; r<OLED_CHAR_ROWS; r++){
    oled_buff[r][0]=0;
  }
}

void fontchip_select_addr(char c){
  if(fontchip_enabled) fontchip_deselect();

  gpio_clear(GPIOB, GPIO12);
  for(int i = 0; i<350; i++)__asm__("NOP");
  fontchip_enabled = true;

  uint32_t addr = ((c-0x20)*8)+OLED_FONT_MEM_OFFSET;
  spi_send(SPI2, 0x0B);
  spi_send(SPI2, (addr>>16)&0xFF);
  spi_send(SPI2, (addr>>8)&0xFF);
  spi_send(SPI2, (addr>>0)&0xFF);
  spi_send(SPI2, 0xAA);//dummy write
  for(int i = 0; i<200; i++)__asm__("NOP");
  spi_xfer(SPI2, 0xAA);//This is somehow necessary to return the correct column
}

void fontchip_deselect(void){
  gpio_set(GPIOB, GPIO12);
  for(int i = 0; i<350; i++)__asm__("NOP");
  fontchip_enabled = false;
}

void oled_send(uint8_t dat){
  oled_data_mode();
  spi_send(SPI1, dat);
  for(int i = 0; i<350; i++)__asm__("NOP");
}

static void _oled_write_char(uint8_t);
static void _oled_write_char(uint8_t c){
  fontchip_select_addr(c);

  for(int _col=0; _col<OLED_CHAR_WIDTH; _col++)
    oled_send(spi_xfer(SPI2, 0x00));

  for(int _col=0; _col<OLED_CHAR_X_PADDING; _col++)
    oled_send(0);

  fontchip_deselect();
}

static void show_oled_buff(char c){
  if(c=='\n'){
    iprintf("\\n");
  }else if(c=='\r'){
    iprintf("\\r");
  }else{
    iprintf("%c ", c);
  }
  iprintf(" r_start:%d r_end:%d col:%d\r\n"
	  "    *********************\r\n"
	  "0) '%s'\r\n"
	  "1) '%s'\r\n"
	  "2) '%s'\r\n"
	  "3) '%s'\r\n", oled_buff_row_start,
	  oled_buff_row_end, oled_buff_col_cursor,
	  oled_buff[0],oled_buff[1],oled_buff[2],oled_buff[3]);
}

static void oled_char_clear_screen(void){
  oled_buff_row_start = 0;
  oled_buff_row_end = 0;
  oled_buff_col_cursor = 0;
  oled_buff_active_rows = 1;

  for(unsigned int r=0; r<OLED_CHAR_ROWS; r++){
    oled_buff[r][0] = 0;
    Set_Column(0);
    Set_Page(r);
    for(unsigned int col=0; col<OLED_CHAR_COLS; col++)
      _oled_write_char(' ');
  }

  Set_Column(0);
  Set_Page(0);
}

static inline void _oled_increment_char_col_row(void){
  oled_buff_col_cursor = 0;
  oled_buff_row_end = (oled_buff_row_end+1) % OLED_CHAR_ROWS;
  if(oled_buff_row_start==oled_buff_row_end)
    oled_buff_row_start = (oled_buff_row_end+1) % OLED_CHAR_ROWS;
}

static bool newline_from_width = false;
void oled_write_char(uint8_t c){
  if(c == '\f') return oled_char_clear_screen();
  if(c == '\r') return;
  if(c < 0x20 && c != '\n') c=' ';

  bool did_newline = false;
  if(c != '\n'){
    if(newline_from_width) did_newline = true;
    newline_from_width = false;
    oled_buff[oled_buff_row_end][oled_buff_col_cursor++] = c;
    oled_buff[oled_buff_row_end][oled_buff_col_cursor] = 0;
  }
  if(oled_buff_col_cursor == OLED_CHAR_COLS){
    newline_from_width = true;
    _oled_increment_char_col_row();
  }
  if(c == '\n'){
    did_newline = true;
    newline_from_width = false;
    oled_buff[oled_buff_row_end][oled_buff_col_cursor] = 0;
    if(!newline_from_width){
      _oled_increment_char_col_row();
      oled_buff[oled_buff_row_end][oled_buff_col_cursor] = 0;
    }
  }

  show_oled_buff(c);

  unsigned int last_row_col = 0;
  if(did_newline){
    //redraw everything
    iprintf("redraw\r\n");
    if(oled_buff_active_rows <OLED_CHAR_ROWS) oled_buff_active_rows++;
    for(unsigned int r = 0; r<oled_buff_active_rows; r++){
      int rownum = (oled_buff_row_start+r) % OLED_CHAR_ROWS;
      int col = 0;
      iprintf("redraw r: %d buffr: %d\r\n", r, rownum);

      Set_Column(0);
      Set_Page(r);

      while (oled_buff[rownum][col] != 0)
	_oled_write_char(oled_buff[rownum][col++]);
      last_row_col = col;
      for(; col<OLED_CHAR_COLS; col++)
	_oled_write_char(' ');
    }
    Set_Char_Column(last_row_col);
  } else
    _oled_write_char(c);
}
