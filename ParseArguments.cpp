#include "ParseArguments.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

ParseArguments::ParseArguments(int argc, char* argv[]) {
    parseArguments(argc, argv);
}

string ParseArguments::toString() const {
    return "http method: " + ::toString(_httpMethod) + "\n" +
        "url: " + _url + "\n" +
        "data: " + _data + "\n" +
        "data file name: " + _dataFileName + "\n" +
        "num threads: " + ::to_string(_numThreads) + "\n" +
        "is test run: " + (_isTestRun ? "true" : "false") + "\n" +
        "output format: " + _outputFormat + "\n" +
        "time out: " + ::to_string(_timeOut) + "\n" +
        "call count: " + ::to_string(_callCount);
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
    _timeOut = 0;
    _callCount = 1;

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
        } else if (0 == arg.compare("-cc") || 0 == arg.compare("--call-count")) {
            if (++cur < argc) {
                _callCount = atoi(argv[cur]);
            }
        } else {
            arguments.push_back(arg);
        }
        cur += 1;
    }
    return arguments;
}
