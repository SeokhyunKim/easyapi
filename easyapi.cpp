#include <iostream>
#include <string>
#include <cstring>
#include <cstddef>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>
#include "HttpCall.hpp"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

vector<string> extractVariables(const string& templateStr) {
    size_t pos = templateStr.find("${");
    if (pos == string::npos) {
        return vector<string>();
    }
    vector<string> variables;
    while (pos != string::npos) {
        size_t endPos = templateStr.find("}", pos + 2);
        if (endPos != string::npos && (endPos - pos - 2 > 0)) {
            variables.push_back(templateStr.substr(pos + 2, endPos - pos - 2));
        }
        pos = templateStr.find("${", pos + 2);
    }
    return variables;
}

vector<string> tokenizeCSVLine(const string& line) {
    char* lineary = (char*)line.c_str();
    char* token = std::strtok(lineary, " ,");
    vector<string> tokens;
    while (token != nullptr) {
        tokens.push_back(string(token));
        token = std::strtok(nullptr, " ,");
    }
    return tokens;
}

bool isSame(const vector<string>& vec1, const vector<string>& vec2) {
    if (vec1.size() != vec2.size()) {
        return false;
    }
    for (auto& str1 : vec1) {
        bool find = false;
        for (auto& str2 : vec2) {
            if (0 == str1.compare(str2)) {
                find = true;
                break;
            }
        }
        if (!find) {
            return false;
        }
    }
    return true;
}


int main(int argc, char* argv[]) {
    if (argc < 3 || 0 == strcmp(argv[1], "help")) {
        cout << "usage: easyapi get|post|put|delete url [data json] [file name for generating different data jsons] [number of threads]" << endl << endl;
        cout << "- Simple one call cases" << endl;
        cout << "  # just give url and data. Currently, only support json for data. (will be extended later)" << endl;
        cout << "  easyapi get http://blabla.com/api/test" << endl;
        cout << "  easyapi post http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl;
        cout << "  easyapi put http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl;
        cout << "  easyapi delete post http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl << endl;
        cout << "- Making multiple calls using variables and data file" << endl;
        cout << "  # use ${var} in url path or data." << endl;
        cout << "  # in below example, ${var1} and ${var2} will be replaced with the values in data_file." << endl;
        cout << "  # and also 5 threads will be used to make calls simulataneously." << endl;
        cout << "  easyapi post http://blabla.com/api/${var1}/test '{\"key\":\"${var2}\"}' data_file 5" << endl;
        cout << "- about data file format" << endl;
        cout << "  # see this example" << endl;
        cout << "  var1, var2, var3   # first line should have list of variables. These variables also shuld exist in get-url path or data json of post call" << endl;
        cout << "  11, 22, 33         # var1, var2, var3 of get-url or post data-json will be replaced with these values." << endl;
        cout << "  ...                # number of calls should be same with number of these value lines." << endl;
        cout << "  ...                # If multiple threads are used, those will make calls by dividing given values" << endl;
        return 0;
    }

    HttpCall httpCall;
    int key = httpCall.createKey();

    HttpMethod method = fromString(string(argv[1]));
    if (method == UNDEFINED) {
        cout << "Currently, support get, post, put, and delete calls." << endl;
        cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
        return 0;
    }

    string path = argv[2];
    vector<string> pathVariables = extractVariables(path);
    string data;
    vector<string> dataVariables;
    if (argc > 3) {
        data = argv[3];
        dataVariables = extractVariables(data);
    } else {
        data = "";
    }

    if (pathVariables.empty() && dataVariables.empty()) {
        string result = httpCall.call(key, method, path, data);
        if (!HttpCall::isFailed(result)) {
            json j = json::parse(result);
            cout << j.dump(4) << endl;
        } else {
            cout << result << endl;
        }
    } else {
        if (argc < 5) {
            cout << "Variables are used, but data file is not given." << endl;
            cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
            return 0;
        }
        string data_file = argv[4];
        fstream variableData(data_file);
        int numThreads = 1;
        if (argc > 5) {
            numThreads = atoi(argv[5]);
        }
        // check whether the first line of data_file is matched with given variables;
        string firstLine;
        getline(variableData, firstLine);
        vector<string> firstLineVariables = tokenizeCSVLine(firstLine);
        vector<string> variables(pathVariables.begin(), pathVariables.end());
        variables.insert(variables.begin(), dataVariables.begin(), dataVariables.end());
        if (!isSame(variables, firstLineVariables)) {
            cout << "Variables of data-template and data-file are not matched" << endl;
            return 0;
        }

        // read lines from data_file, show configuration, and getting confirmation to go forward
        string line;
        vector<string> lines; 
        while (getline(variableData, line)) {
            lines.push_back(line);
        }
        cout << "Number of data-file lines: " << lines.size() << endl;
        cout << "Number of threads: " << numThreads << endl << endl;
        cout << "List of variables: ";
        for (auto& var : variables) {
            cout << var << " ";
        }
        cout << endl;
        cout << "The first line of the data file: " << firstLine << endl;
        cout << endl << "Everything is looking good? (y/n) ";
        char isProceed;
        cin >> isProceed;
        if (isProceed != 'y' && isProceed != 'Y') {
            return 0;
        }

        // divide chuncks by numThreads and launch workers
        long chunkSize = lines.size() / numThreads;
        auto worker = [=, &httpCall](const vector<string>& lines, const vector<string>& variables, int start, int end) {
            int key = httpCall.createKey();
            for (int i=start; i<end; ++i) {
                string line = lines[i];
                string pathTemplate = path;
                string varjsonTemplate = data;
                vector<string> tokens = tokenizeCSVLine(line);
                for (int i=0; i<variables.size(); ++i) {
                    string replacement = "${" + pathVariables[i] + "}";
                    string value = tokens[i];
                    size_t pos1= pathTemplate.find(replacement);
                    if (pos1 != string::npos) {
                        pathTemplate.replace(pos1, replacement.size(), value);
                    }
                    size_t pos2 = varjsonTemplate.find(replacement);
                    if (pos2 == string::npos) {
                        varjsonTemplate.replace(pos2, replacement.size(), data);
                    }
                }
                string result = httpCall.call(key, method, pathTemplate, varjsonTemplate);
                if (!HttpCall::isFailed(result)) {
                    json jsonResult = json::parse(result);
                    cout << jsonResult.dump(4) << endl;
                } else {
                    cout << result << endl;
                }
            }
        };
        vector<thread> threads;
        for (int i=0; i<numThreads; ++i) {
            threads.push_back(thread(worker, lines, firstLineVariables, i*chunkSize, std::min((i+1)*chunkSize, (long)lines.size())));
        }
        for (auto& th : threads) {
            th.join();
        }
    }

    return 0;
}
