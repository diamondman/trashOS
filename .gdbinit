file trashcan.elf
target remote localhost:3333
set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4


#http://openocd.org/doc/html/GDB-and-OpenOCD.html
define hook-step
  mon cortex_m maskisr on
end
define hookpost-step
  mon cortex_m maskisr off
end

monitor reset halt

load
