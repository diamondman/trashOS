
typedef void thread_func(void);

//void sv_call_handler(void);
void sv_call_write_data(char*, int);
void sv_call_handler_main(unsigned int *);
void sv_call_start_thread(thread_func*);
