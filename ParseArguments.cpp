#include "ParseArguments.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

ParseArguments::ParseArguments(int argc, char* argv[]) {
    parseArguments(argc, argv);
}

string ParseArguments::toString() const {
    string headers;
    for (const auto& header : _headers) headers += header + " ";
    return "http method: " + ::toString(_httpMethod) + "\n" +
        "url: " + _url + "\n" +
        "data: " + _data + "\n" +
        "data file name: " + _dataFileName + "\n" +
        "num threads: " + ::to_string(_numThreads) + "\n" +
        "is test run: " + (_isTestRun ? "true" : "false") + "\n" +
        "output format: " + _outputFormat + "\n" +
        "time out: " + ::to_string(_timeOut) + "\n" +
        "delimiters: " + _delimiters + "\n" +
        "is force run: " + (_isForceRun ? "true" : "false") + "\n" +
        "num api calls: " + ::to_string(_numApiCalls) + "\n" +
        "headers: " + headers;
}

void ParseArguments::parseArguments(int argc, char* argv[]) {
    // default parameters
    _httpMethod = GET;
    _url = "";
    _data = "";
    _dataFileName = "";
    _numThreads = 1;
    _isTestRun = false;
    _outputFormat = "json";
    _delimiters = " ,";
    _isForceRun = false;
    _timeOut = 0;
    _numApiCalls = 0;
    _headers.clear();

    // update optional parameters and get vector of mandatory ones
    vector<string> arguments = extractOptionalArguments(argc, argv);
    if (arguments.size() >= 1) {
        _httpMethod = fromString(arguments[0]);
    }
    if (arguments.size() >= 2) {
        _url = arguments[1];
    }
    if (arguments.size() >= 3) {
        _data = arguments[2];
    }
}

vector<string> ParseArguments::extractOptionalArguments(int argc, char* argv[]) {
    vector<string> arguments;
    int cur = 1;
    while (cur < argc) {
        string arg(argv[cur]);
        if (0 == arg.compare("-f") || 0 == arg.compare("--file")) {
            if (++cur < argc) {
                _dataFileName = argv[cur];
            }
        } else if (0 == arg.compare("-nt") || 0 == arg.compare("--num-threads")) {
            if (++cur < argc) {
                _numThreads = atoi(argv[cur]);
            }
        } else if (0 == arg.compare("-tr") || 0 == arg.compare("--test-run")) {
            _isTestRun = true;
        } else if (0 == arg.compare("-to") || 0 == arg.compare("--time-out")) {
            if (++cur < argc) {
                _timeOut = atoi(argv[cur]);
            }
        } else if (0 == arg.compare("-of") || 0 == arg.compare("--output-format")) {
            if (++cur < argc) {
                _outputFormat = argv[cur];
            }
        } else if (0 == arg.compare("-d") || 0 == arg.compare("--delimiters")) {
            if (++cur < argc) {
                _delimiters = argv[cur];
            }
        } else if (0 == arg.compare("-fr") || 0 == arg.compare("--force-run")) {
            _isForceRun = true;
        } else if (0 == arg.compare("-nc") || 0 == arg.compare("--num-api-calls")) {
            if (++cur < argc) {
                _numApiCalls = atoi(argv[cur]);
            }
        } else if (0 == arg.compare("-h") || 0 == arg.compare("--header")) {
            if (++cur < argc) {
                _headers.push_back(string(argv[cur]));
            }
        } else {
            arguments.push_back(arg);
        }
        cur += 1;
    }
    return arguments;
}
