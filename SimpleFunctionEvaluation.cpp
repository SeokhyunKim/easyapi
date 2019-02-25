#include "SimpleFunctionEvaluation.hpp"

using namespace std;

bool isAlphabet(char c) {
    int v = (int)c;
    // a ~ z or A ~ Z
    return (97 <= v && v <= 122) || (65 <= v && v <= 90);
}

bool isNumber(char c) {
    int v = (int)c;
    // 0 ~ 9
    return (48 <= v && v <= 57);
}

bool SimpleFunctionEvaluation::isFunctionExpression(const string& text) {
    const char* letters = text.c_str();
    string::size_type length = text.size();
    if (length < 1 || letters[0] != '=') {
        return false;
    }
    string fnNames[] = {"rand"};
    int state = 0;
    int cur = 1;
    while (cur < length)
        char c = letters[cur];
        switch(state) {
            case 0: // function
                if (!isAlphabet(c)) {
                    return false;
                }
                state = 1;
                ++cur;
                break;
            case 1: // function name
                // todo: check string from cur whether it is one of supported fn names
                state = 2;
                // todo: adjust cur properly
                break;
            case 2: // argument list
                if (c == '(') {
                    state = 3;
                    ++cur;
                    continue;
                } else {
                    return false;
                }
                break;
            case 3: // argument
                if (isAlphabet(c)) {
                    state = 0; // argument accepts another function
                    continue;
                } else if (isNumber(c)) {
                    state = 4;
                    contunue;
                } else {
                    return false;
                }


        }
    }
}

string SimpleFunctionEvaluation::evaluate(const string& fnExp) {
}

double SimpleFunctionEvaluation::evaluate(const string& fnExp) {
}

long SimpleFunctionEvaluation::evaluate(const string& fnExp) {
}
