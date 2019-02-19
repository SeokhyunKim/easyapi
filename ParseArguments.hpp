#ifndef _EASYAPI_PARSE_ARGUMENTS_HPP_
#define _EASYAPI_PARSE_ARGUMENTS_HPP_

#include <vector>
#include <string>
#include "HttpCall.hpp"

using namespace std;

class ParseArguments {
public:
    ParseArguments(int argc, char* argv[]);

    HttpMethod getMethod() const { return _httpMethod; }
    string getUrl() const { return _url; }
    string getData() const { return _data; }
    string getDataFileName() const { return _dataFileName; }
    int getNumThreads() const { return _numThreads; }

private:
    void parseArguments(int argc, char* argv[]);
    vector<string> extractOptionalArguments(int argc, char* argv[]);

private:
    // mandatory parameters
    HttpMethod _httpMethod;
    string _url;
    string _data;

    // optional parameters
    string _dataFileName;
    int _numThreads;
    bool _isTestRun;

};

#endif

