#include "func_util.h"
#include <iostream>

using namespace std;

// super primitive testings without testing framework.
// will use google test later.
extern "C" {
extern double rand_ratio();
extern double rand_0(double args[], int length);
extern double rand_1(double args[], int length);
extern double rand_2(double args[], int length);
}

void test_rand_ratio() {
    cout << "test_rand_ratio()" << endl;
    for (int i=0; i<10; ++i) {
        cout << rand_ratio() << endl;
    }
    cout << endl;
}

void test_rand_functions() {
    cout << "test_rand_functions()" << endl;
    cout << "rand_0" << endl;
    for (int i=0; i<10; ++i) {
        cout << rand_0({}, 0) << endl;
    }
    cout << endl;
    cout << "rand_1(10)" << endl;
    for (int i=0; i<10; ++i) {
        double args[] = {10.0};
        cout << rand_1(args, 1) << endl;
    }
    cout << endl;
    cout << "rand_2(10, 15)" << endl;
    for (int i=0; i<10; ++i) {
        double args[] = {10.0, 15.0};
        cout << rand_2(args, 2) << endl;
    }
    cout << endl;
}

/* todo: update tests
void test_evaluate_func_call() {
    cout << "test_evaluate_func_call()" << endl;
    clear_func_calls();
    cout << "rand0 case" << endl;
    func_call* fc = assign_func_call();
    strcpy(fc->func_name, "rand");
    fc->num_arguments = 0;
    for (int i=0; i<10; ++i) {
        func_call_result result = evaluate_func_call(fc);
        if (result.is_success) {
            cout << "result: " << result.value << endl;
        }
    }
    cout << "rand1 case" << endl;
    fc = assign_func_call();
    strcpy(fc->func_name, "rand");
    fc->num_arguments = 1;
    fc->arguments[0] = 10.0;
    for (int i=0; i<10; ++i) {
        func_call_result result = evaluate_func_call(fc);
        if (result.is_success) {
            cout << "result: " << result.value << endl;
        }
    }
    cout << "rand2 case" << endl;
    fc = assign_func_call();
    strcpy(fc->func_name, "rand");
    fc->num_arguments = 2;
    fc->arguments[0] = 10.0;
    fc->arguments[1] = 15.0;
    for (int i=0; i<10; ++i) {
        func_call_result result = evaluate_func_call(fc);
        if (result.is_success) {
            cout << "result: " << result.value << endl;
        }
    }
    cout << endl;
}*/

int main() {
    test_rand_ratio();
    test_rand_functions();
    //test_evaluate_func_call();

    return 0;
}
