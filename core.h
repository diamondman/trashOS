unsigned int __get_PSP(void);
void __set_PSP(unsigned int);
unsigned int __get_MSP(void);
void __set_MSP(unsigned int);
unsigned int __get_CONTROL(void);
void sv_goto_process(void);
//void __start_critical(void);
//void __end_critical(void);
void osSwitchThreadStack(unsigned int, unsigned int);
