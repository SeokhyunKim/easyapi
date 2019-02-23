#ifndef _EASYAPI_PARSE_ARGUMENTS_HPP_
#define _EASYAPI_PARSE_ARGUMENTS_HPP_

#include <vector>
#include <string>
#include "HttpCall.hpp"

class ParseArguments {
public:
    ParseArguments(int argc, char* argv[]);

    HttpMethod getMethod() const { return _httpMethod; }
    std::string getUrl() const { return _url; }
    std::string getData() const { return _data; }
    std::string getDataFileName() const { return _dataFileName; }
    int getNumThreads() const { return _numThreads; }
    bool isTestRun() const { return _isTestRun; }
    std::string toString() const;

private:
    void parseArguments(int argc, char* argv[]);
    std::vector<std::string> extractOptionalArguments(int argc, char* argv[]);

private:
    // mandatory parameters
    HttpMethod _httpMethod;
    std::string _url;
    std::string _data;

    // optional parameters
    std::string _dataFileName;
    int _numThreads;
    bool _isTestRun;

};

#endif

