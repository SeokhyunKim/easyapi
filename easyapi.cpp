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
        cout << "usage: easyapi get|post url [data json] [variable name] [file name for the variable]" << endl;
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
