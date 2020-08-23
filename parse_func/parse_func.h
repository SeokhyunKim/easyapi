#ifndef _EASYAPI_EVAL_FUNC_H_
#define _EASYAPI_EVAL_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

long parse_func(const char* str);
char* get_error_message();
int is_parse_func_succeeded();

#ifdef __cplusplus
}
#endif

#endif
