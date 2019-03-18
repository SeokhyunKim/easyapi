#ifndef _EASYAPI_FUNC_UTIL_H_
#define _EASYAPI_FUNC_UTIL_H_

/* Later, this will be updated to c++ */

struct func_call_result {
    int is_success;
    double value;
};

void init_func_call();

/* returns 1 for success, and 0 for failure. (this is C code) */
int add_arg(double d);

struct func_call_result eval_func_call(char* func_name);

#endif
