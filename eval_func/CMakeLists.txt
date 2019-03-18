
add_library(eval_func lex.yy.c func_util.c func.tab.c)
add_executable(tests tests.cpp)

target_link_libraries(tests eval_func)
