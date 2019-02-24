#include <iostream>
#include <string>
#include <cstring>
#include <cstddef>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include "HttpCall.hpp"
#include "ParseArguments.hpp"
#include <nlohmann/json.hpp>

using namespace std;
using namespace std::chrono;
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

void printHttpCallResponse(HttpCallResponse& response) {
    if (response.code == 200l) {
        json j = json::parse(response.response);
        cout << j.dump(4) << endl;
    } else if (response.code > 0l) {
        cout << "code: " << response.code << ", response: " << response.response << endl; 
    } else {
        cout << response.response << endl;
    }
}

string getTestCommand(HttpMethod method, const string& url, const string& data) {
    if (!data.empty()) {
        return "easyapi " + ::toString(method) + " " + url + " " + data;
    }
    return "easyapi " + ::toString(method) + " " + url;
}

void processSingleApiCall(const ParseArguments& pa) {
    if (pa.isTestRun()) {
        cout << getTestCommand(pa.getMethod(), pa.getUrl(), pa.getData()) << endl;
        return;
    }
    HttpCall httpCall;
    int key = httpCall.createKey();
    HttpCallResponse response = httpCall.call(key, pa.getMethod(), pa.getUrl(), pa.getData());
    printHttpCallResponse(response);
}

// api call thread worker
void apiCallWorker(const ParseArguments& pa, HttpCall& httpCall,
                   const vector<string>& lines, const vector<string>& variables, int start, int end,
                   int& callCount, long& elappsedTime) {
    int key = httpCall.createKey();
    for (int i=start; i<end; ++i) {
        string line = lines[i];
        string pathTemplate = pa.getUrl();
        string varjsonTemplate = pa.getData();
        vector<string> tokens = tokenizeCSVLine(line);
        for (int i=0; i<variables.size(); ++i) {
            string replacement = "${" + variables[i] + "}";
            string value = tokens[i];
            size_t pos1= pathTemplate.find(replacement);
            if (pos1 != string::npos) {
                pathTemplate.replace(pos1, replacement.size(), value);
            }
            size_t pos2 = varjsonTemplate.find(replacement);
            if (pos2 != string::npos) {
                varjsonTemplate.replace(pos2, replacement.size(), value);
            }
        }
        if (pa.isTestRun()) {
            cout << getTestCommand(pa.getMethod(), pathTemplate, varjsonTemplate) << endl;
        } else {
            auto start = high_resolution_clock::now();
            HttpCallResponse response = httpCall.call(key, pa.getMethod(), pathTemplate, varjsonTemplate);
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            elappsedTime += duration.count();
            printHttpCallResponse(response);
        }
        callCount += 1;
    }
}

// thread worker for printing out average api latency
void latencyChecker(const vector<int>& callCounts, const vector<long>& elappsedTimes, bool& isDone) {
    auto last = system_clock::now();
    while (!isDone) {
        auto current = system_clock::now();
        auto duration = duration_cast<seconds>(current - last);
        if (duration.count() < 2) {
            this_thread::sleep_for(milliseconds(500));
            continue;
        }
        long totalTime = 0l;
        for_each(elappsedTimes.begin(), elappsedTimes.end(), [&totalTime](long t) { totalTime += t; });
        int totalCount = 0;
        for_each(callCounts.begin(), callCounts.end(), [&totalCount](int c) { totalCount += c; });
        if (totalTime <= 0l || totalCount <= 0l) {
            this_thread::sleep_for(milliseconds(100));
            continue;
        }
        double avgTime = (double)totalTime / totalCount;
        cout << "Average time for api call: " << avgTime <<
                " ms, Total call count: " << totalCount <<
                ", Total elappsed time: " << totalTime <<
                " ms" << endl;
        last = current;
    }
}

void processMultipleApiCalls(const ParseArguments& pa,
                             const vector<string>& pathVariables,
                             const vector<string>& dataVariables,
                             fstream& variableData) {
    HttpCall httpCall;

    // check whether the first line of data_file is matched with given variables;
    string firstLine;
    getline(variableData, firstLine);
    vector<string> firstLineVariables = tokenizeCSVLine(firstLine);
    vector<string> variables(pathVariables.begin(), pathVariables.end());
    variables.insert(variables.begin(), dataVariables.begin(), dataVariables.end());
    if (!isSame(variables, firstLineVariables)) {
        cout << "Variables of data-template and data-file are not matched" << endl;
        return;
    }
    // read lines from data_file, show configuration, and getting confirmation to go forward
    string line;
    vector<string> lines; 
    while (getline(variableData, line)) {
        lines.push_back(line);
    }
    // print parameters before executing multiple threads for making lots of api calls.
    cout << "Number of data-file lines: " << lines.size() << endl;
    cout << "Number of threads: " << pa.getNumThreads() << endl << endl;
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
        return;
    }

    // divide chuncks by numThreads and launch workers
    long chunkSize = lines.size() / pa.getNumThreads();
    if (chunkSize <= 0l) {
        cout << "Too many threads. Number of input lines: " << lines.size() << ", Thread number: " << pa.getNumThreads() << endl;
        return;
    }

    // starting threads for multiple api calls
    vector<thread> threads;
    vector<int> callCounts(pa.getNumThreads());
    vector<long> elappsedTimes(pa.getNumThreads());
    for (int i=0; i<pa.getNumThreads(); ++i) {
        callCounts.push_back(0);
        elappsedTimes.push_back(0l);
        threads.push_back(thread(apiCallWorker, ref(pa), ref(httpCall), ref(lines), ref(firstLineVariables), i*chunkSize, std::min((i+1)*chunkSize, (long)lines.size()),
                                 ref(callCounts[i]), ref(elappsedTimes[i])));
    }
    // This is for printing out call counts per second
    bool isDone = false;
    thread callCountWorker;
    if (!pa.isTestRun()) {
        callCountWorker = thread(latencyChecker, ref(callCounts), ref(elappsedTimes), ref(isDone));
    }
    // joining worker threads
    for (auto& th : threads) {
        th.join();
    }
    isDone = true;
    if (callCountWorker.joinable()) {
        callCountWorker.join();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3 || 0 == strcmp(argv[1], "help")) {
        cout << "usage: easyapi get|post|put|delete url [data json] [optional parameters]" << endl << endl;
        cout << "- Simple one call cases" << endl;
        cout << "  # just give url and data. Currently, only support json for data. (will be extended later)" << endl;
        cout << "  easyapi get http://blabla.com/api/test" << endl;
        cout << "  easyapi post http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl;
        cout << "  easyapi put http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl;
        cout << "  easyapi delete post http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl << endl;
        cout << "- Making multiple calls using variables and data file" << endl;
        cout << "  # use ${var} in url path or data." << endl;
        cout << "  # in below example, ${var1} and ${var2} will be replaced with the values in data_file." << endl;
        cout << "  easyapi post http://blabla.com/api/${var1}/test '{\"key\":\"${var2}\"}' --data-file data_file" << endl << endl;
        cout << "  easyapi get http://blabla.com/api/${var1} --data-file data_file" << endl << endl;
        cout << "- About data file format" << endl;
        cout << "  # see this example" << endl;
        cout << "  var1, var2, var3   # first line should have list of variables. These variables also shuld exist in get-url path or data json of post call" << endl;
        cout << "  11, 22, 33         # var1, var2, var3 of get-url or post data-json will be replaced with these values." << endl;
        cout << "  ...                # number of calls should be same with number of these value lines." << endl;
        cout << "  ...                # If multiple threads are used, those will make calls by dividing given values" << endl << endl;
        cout << "- Optional parameters" << endl;
        cout << "  --data-file or -f" << "\t" << "Data file name containing list of path and data variables at the first line, and real datas in following lines." << endl;
        cout << "  --test-run or -tr" << "\t" << "Test run. Don't make real api calls. Instead showing command which will be used for real calls." << endl;
        cout << "  --num-threads or -nt" << "\t" << "Number of threads used for making multiple calls with multiple threads" << endl;
        return 0;
    }

    ParseArguments pa(argc, argv);

    if (pa.getMethod() == UNDEFINED) {
        cout << "Currently, support get, post, put, and delete calls." << endl;
        cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
        return 0;
    }

    vector<string> pathVariables = extractVariables(pa.getUrl());
    vector<string> dataVariables = extractVariables(pa.getData());

    if (pathVariables.empty() && dataVariables.empty()) {
        processSingleApiCall(pa);
    } else {
        // todo: add test to check existence of the file
        if (pa.getDataFileName().empty()) {
            cout << "Variables are used, but data file is not given." << endl;
            cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
            return 0;
        }
        fstream variableData(pa.getDataFileName());
        processMultipleApiCalls(pa, pathVariables, dataVariables, variableData);
    }

    return 0;
}
