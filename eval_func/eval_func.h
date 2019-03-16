#ifndef _EASYAPI_EVAL_FUNC_H_
#define _EASYAPI_EVAL_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

long evaluate_function(char* str);
char* get_error_message();
int is_evaluation_succeeded();

#ifdef __cplusplus
}
#endif

#endif
