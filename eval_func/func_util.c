#include "func_util.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define FUNC_NAME_LENGTH    100
#define NUM_ARGUMENTS       10

/*#define _FUNC_DEBUG*/

struct {
    char func_name[FUNC_NAME_LENGTH];
    double arguments[NUM_ARGUMENTS];
    int num_arguments;
} func_call;

typedef double (*FUNC)(double args[], int length);

double rand_0(double args[], int length);
double rand_1(double args[], int length);
double rand_2(double args[], int length);

struct {
    char func_name[100];
    int num_arguments;
    FUNC func_ptr;
} funcs[] = {
    { "rand", 0, rand_0 },
    { "rand", 1, rand_1 },
    { "rand", 2, rand_2 }
};

void init_func_call() {
    time_t t;
    srand((unsigned) time(&t));
}

void clear_func_call() {
    func_call.func_name[0] = '\0';
    func_call.num_arguments = 0;
}

int add_arg(double d) {
    if (func_call.num_arguments >= NUM_ARGUMENTS) {
        return 0;
    }
    func_call.arguments[func_call.num_arguments++] = d;
#ifdef _FUNC_DEBUG
    printf("arg num: %d\n", func_call.num_arguments);
#endif
    return 1;
}

struct func_call_result eval_func_call(char* s) {
    strncpy(func_call.func_name, s, sizeof(func_call.func_name));
    int numFuncs = (int)sizeof(funcs) / sizeof(funcs[0]);
    for (int i=0; i<numFuncs; ++i) {
        if (0 == strcmp(func_call.func_name, funcs[i].func_name) &&
            func_call.num_arguments == funcs[i].num_arguments) {
            double value = (*(funcs[i].func_ptr))(func_call.arguments, func_call.num_arguments);
            struct func_call_result result = { 1, value };
            return result;
        }
    }
    struct func_call_result result = { 0, 0 };
    clear_func_call();
    return result;
}

double rand_ratio() {
    int r = rand();
    if (r >= RAND_MAX) {
        r -= 1;
    }
    return (double)r / RAND_MAX;
}

double rand_0(double args[], int length) {
#ifdef _FUNC_DEBUG
    printf("rand_0\n");
#endif
    return rand();
}

double rand_1(double args[], int length) {
#ifdef _FUNC_DEBUG
    printf("rand_1\n");
#endif
    return rand_ratio() * args[0];
}

double rand_2(double args[], int length) {
#ifdef _FUNC_DEBUG
    printf("rand_2\n");
#endif
    double len = args[1] - args[0];
    return rand_ratio() * len + args[0];
}
