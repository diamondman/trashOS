#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include <sys/unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "oled.h"

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

