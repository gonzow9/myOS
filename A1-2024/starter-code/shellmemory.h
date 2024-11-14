#define MEM_SIZE 1000

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 30  // Default value if not defined
#endif

#ifndef VARIABLE_STORE_SIZE
#define VARIABLE_STORE_SIZE 100  // Default value if not defined
#endif

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
