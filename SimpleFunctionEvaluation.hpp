#ifndef _EASYAPI_SIMPLE_FUNCTION_EVALUATION_HPP_
#define _EASYAPI_SIMPLE_FUNCTION_EVALUATION_HPP_

#include <string>

class SimpleFunctionEvaluation {
public:
    SimpleFunctionEvaluation() {}

    bool isFunctionExpression(const string& text);

    std::string evaluate(const string& fnExp);
    double evaluate(const string& fnExp);
    long evaluate(const string& fnExp);

};

#endif
