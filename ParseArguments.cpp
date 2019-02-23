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
        "is test run: " + (_isTestRun ? "true" : "false") + "\n";
}

void ParseArguments::parseArguments(int argc, char* argv[]) {
    // default parameters
    _httpMethod = GET;
    _url = "";
    _data = "";
    _dataFileName = "";
    _numThreads = 1;
    _isTestRun = false;

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
        if (0 == arg.compare("-f") || 0 == arg.compare("--data-file")) {
            if (++cur < argc) {
                _dataFileName = argv[cur];
            }
        } else if (0 == arg.compare("-nt") || 0 == arg.compare("--num-threads")) {
            if (++cur < argc) {
                _numThreads = atoi(argv[cur]);
            }
        } else if (0 == arg.compare("-tr") || 0 == arg.compare("--test-run")) {
            _isTestRun = true;
        } else {
            arguments.push_back(arg);
        }
        cur += 1;
    }
    return arguments;
}
