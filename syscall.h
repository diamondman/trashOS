
typedef void thread_func(void);

//void sv_call_handler(void);
void sv_call_write_data(char*, int);
void sv_call_handler_main(unsigned int *);
void sv_call_start_thread(thread_func*);

/*
 * Handler function definition, same parameters as the SVC function below
 */
void sv_call_write_data_handler(char *string, int length);
void sv_call_start_thread_handler(unsigned int *svc_args, thread_func* func);


#define SVC_WRITE_DATA 1
#define SVC_START_THREAD 2



/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) __asm volatile ("svc %[immediate]"::[immediate] "I" (code))

typedef struct {
  unsigned int *stack;
  unsigned int *stackTop;
  unsigned int status;
} Thread;

#define TCB_LEN 8
#define TCB_THREAD_INUSE 0x80000000
#define STACK_WORD_SIZE (102)
