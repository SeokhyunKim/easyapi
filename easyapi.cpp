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

vector<string> extractVariables(const string& jsonTemplate) {
    size_t pos = jsonTemplate.find("${");
    if (pos == string::npos) {
        return vector<string>();
    }
    vector<string> variables;
    while (pos != string::npos) {
        size_t endPos = jsonTemplate.find("}", pos + 2);
        if (endPos != string::npos && (endPos - pos - 2 > 0)) {
            variables.push_back(jsonTemplate.substr(pos + 2, endPos - pos - 2));
        }
        pos = jsonTemplate.find("${", pos + 2);
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
        cout << "usage: easyapi get|post url [data json] [file name for generating different data jsons] [number of threads]" << endl;
        cout << "- get cases" << endl;
        cout << "  # this will return respons of this get api call" << endl;
        cout << "  easyapi get http://blabla.com/api/test" << endl;
        cout << "  # this will make api calls using the values in data_file. The values of data_file is used to replace ${var1} for different calls." << endl;
        cout << "  # also 5 threads will be used to make api calls. If thread number is not given, only one thread is used." << endl;
        cout << "  easyapi get http://blabla.com/api/test/${var1} data_file 5" << endl;
        cout << "- post cases" << endl;
        cout << "  # this will make post call with given data" << endl;
        cout << "  easyapi post http://blabla.com/api/post-test/ '{\"field\":\"value\"}'" << endl;
        cout << "  # this will make post calls with populated data jsons using values in data_file. Also, this will use 10 threads." << endl;
        cout << "  easyapi post http://blabla.com/api/post-test/ '{\"field\":\"${var}\"}' 10" << endl;
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
    
    if (0 == strcmp(argv[1], "get")) {
        cout << httpCall.get(key, argv[2]) << endl;
    } else if (0 == strcmp(argv[1], "post")) {
        string url = argv[2];
        if (argc == 4) {
            string data = argv[3];
            string result = httpCall.post(key, url, data);
            if (!HttpCall::isFailed(result)) {
                json j = json::parse(result);
                cout << j.dump(4) << endl;
            } else {
                cout << result << endl;
            }
        } else if (argc <= 6) {
            if (argc < 6) {
                cout << "Not enough parameters for data json having a variable." << endl;
                return 0;
            }
            string jsonTemplate = argv[3];
            vector<string> variables = extractVariables(jsonTemplate);
            string fileName = argv[4];
            int numThreads = atoi(argv[5]);
            fstream variableData(fileName);
            string firstLine;
            getline(variableData, firstLine);
            vector<string> firstLineVariables = tokenizeCSVLine(firstLine);
            if (!isSame(variables, firstLineVariables)) {
                cout << "Variables of data-template and data-file are not matched" << endl;
                return 0;
            }

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

            long chunkSize = lines.size() / numThreads;
            auto worker = [=, &httpCall](const vector<string>& lines, const vector<string>& variables, int start, int end) {
                int key = httpCall.createKey();
                for (int i=start; i<end; ++i) {
                    string line = lines[i];
                    string varjsonTemplate = jsonTemplate;
                    vector<string> tokens = tokenizeCSVLine(line);
                    for (int i=0; i<variables.size(); ++i) {
                        string replacement = "${" + variables[i] + "}";
                        string data = tokens[i];
                        size_t pos = varjsonTemplate.find(replacement);
                        if (pos == string::npos) {
                            continue;
                        }
                        varjsonTemplate.replace(pos, replacement.size(), data);
                    }
                    string result = httpCall.post(key, url, varjsonTemplate);
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
        } else {
            cout << "Wrong options. Try again. argc: " << argc  << endl;
        }
    } else {
        cout << "1st option should be get or post." << endl;
    }

    return 0;
}
