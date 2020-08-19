#include <iostream>
#include <string>
#include <cstring>
#include <cstddef>
#include <cmath>
#include <fstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <exception>
#include "HttpCall.hpp"
#include "ParseArguments.hpp"
#include <nlohmann/json.hpp>
#include "easyapi_util.hpp"
#include "BufferedPrint.hpp"
#include "parse_func/parse_func.h"
#include "parse_func/func_util.h"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

string getHttpCallResponse(
    HttpCallResponse& response, const string& outputFormat) {
    if (response.code == 200l && 0 == outputFormat.compare("json")) {
        json j = json::parse(response.response);
        return j.dump(4);
    } else if (response.code > 0l) {
        return "code: " + to_string(response.code) + ", response: " + response.response;
    }
    return response.response;
}

string getTestCommand(HttpMethod method, const string& url,
                      const string& data) {
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
    HttpCallResponse response = httpCall.call(key, pa.getMethod(), pa.getUrl(), pa.getData(), pa.getTimeOut());
    cout << getHttpCallResponse(response, pa.getOutputFormat()) << endl;
}

string replaceTemplateVariables(
	const string& templateInput, const vector<string>& variables,
	const unordered_map<string, int>& varIndices, const vector<string>& tokens,
        string& variables_string) {
    string replacedTemplate = templateInput;
    for (const string& var : variables) {
        string replacement = "${" + var + "}";
        string value;
        if (var[0] == '=') {
            long result = parse_func(var.c_str());
            if (!is_parse_func_succeeded()) {
                cout << "Failed to evaluate this function: " << var << endl;
                exit(1);
            }
            value = to_string(result);
        } else {
            variables_string += var + " ";
            value = tokens[varIndices.at(var)];
        }
        size_t pos= replacedTemplate.find(replacement);
        replacedTemplate.replace(pos, replacement.size(), value); // pos should be valid
    }
    return replacedTemplate;
}

// api call thread worker
void apiCallWorker(const ParseArguments& pa, HttpCall& httpCall,
                   const vector<string>& lines,
                   const vector<string>& pathVariables,
                   const vector<string>& dataVariables,
                   const unordered_map<string, int>& varIndices,
                   int start, int end, int& callCount, long& elappsedTime,
                   BufferedPrint& bufferedPrint) {
    int key = httpCall.createKey();
    for (int i=start; i<end; ++i) {
        string pathTemplate = pa.getUrl();
        string dataTemplate = pa.getData();
        string line;
        vector<string> tokens;
        if (lines.size() > 0) {
            line = lines[i];
            tokens = tokenizeCSVLine(line);
        }
        string variables_string;
        string replacedPath = replaceTemplateVariables(pathTemplate, pathVariables, varIndices, tokens, variables_string);
        string replacedData = replaceTemplateVariables(dataTemplate, dataVariables, varIndices, tokens, variables_string);
        string inputVars;
        if (variables_string.length() > 0) {
            inputVars = "Input variables: " + variables_string + "\n";
        }
        if (pa.isTestRun()) {
            bufferedPrint.println(inputVars + "Response:\n" + getTestCommand(pa.getMethod(), replacedPath, replacedData));
        } else {
            auto start = high_resolution_clock::now();
            HttpCallResponse response = httpCall.call(key, pa.getMethod(), replacedPath, replacedData, pa.getTimeOut());
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            elappsedTime += duration.count();
            bufferedPrint.println(inputVars + "Response:\n" + getHttpCallResponse(response, pa.getOutputFormat()));
        }
        callCount += 1;
    }
}

// thread worker for printing out average api latency
void latencyChecker(const vector<int>& callCounts, const vector<long>& elappsedTimes, bool& isDone, BufferedPrint& bufferedPrint) {
    auto last = system_clock::now();
    auto startTime = last;
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
        auto currentTime = system_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(currentTime - startTime).count();
        bufferedPrint.println("Average time for api call: " + to_string(avgTime) +
                              " ms, Total call count: " + to_string(totalCount) +
                              ", Elappsed time: " + to_string(elapsedTime) +
                              " ms");
        last = current;
    }
}

void processMultipleApiCalls(const ParseArguments& pa,
                             const vector<string>& pathVariables,
                             const vector<string>& dataVariables) {
    HttpCall httpCall;
    fstream variableData;
    int numPathVariables = getNumVariablesExceptFunctions(pathVariables);
    int numDataVariables = getNumVariablesExceptFunctions(dataVariables);
    if (numPathVariables + numDataVariables > 0) {
        variableData.open(pa.getDataFileName());
    }

    // check whether the first line of data_file is matched with given variables;
    string firstLine;
    vector<string> firstLineVariables;
    vector<string> variables;
    unordered_map<string, int> varIndices;
    if (variableData.is_open()) {
        getline(variableData, firstLine);
        firstLineVariables = tokenizeCSVLine(firstLine);
        int idx = 0;
        for (auto& var : firstLineVariables) {
            varIndices.insert(make_pair(var, idx++));
        }
        variables.insert(variables.begin(), dataVariables.begin(), dataVariables.end());
        variables.insert(variables.begin(), pathVariables.begin(), pathVariables.end());
        vector<string> varsWithoutFuncs = removeFunctions(variables);
        if (!isSame(varsWithoutFuncs, firstLineVariables)) {
            cout << "Variables of data-template and data-file are not matched" << endl;
            return;
        }
    }
    // read lines from data_file, show configuration, and getting confirmation to go forward
    string line;
    vector<string> lines;
    if (variableData.is_open()) {
        while (getline(variableData, line)) {
            lines.push_back(line);
        }
    }
    // print parameters before executing multiple threads for making lots of api calls.
    if (lines.size() > 0) {
        cout << "Number of data-file lines: " << lines.size() << endl;
    } else {
        cout << "Call count: " << pa.getCallCount() << endl;
    }
    cout << "Number of threads: " << pa.getNumThreads() << endl << endl;
    cout << "List of variables or built-in functions: ";
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
    long chunkSize = 0l;
    long callCount = std::max((long)lines.size(), (long)pa.getCallCount());
    chunkSize = (long)ceil((double)callCount / pa.getNumThreads());
    if (chunkSize <= 0l) {
        cout << "Too many threads for call count.. Call count: " << callCount << ", Thread number: " << pa.getNumThreads() << endl;
        return;
    }

    // starting threads for multiple api calls
    vector<thread> threads;
    vector<int> callCounts(pa.getNumThreads());
    vector<long> elappsedTimes(pa.getNumThreads());
    BufferedPrint bufferedPrint(2048);
    for (int i=0; i<pa.getNumThreads(); ++i) {
        callCounts.push_back(0);
        elappsedTimes.push_back(0l);
        long start = i*chunkSize;
        long end = std::min((i+1)*chunkSize, callCount);
        bufferedPrint.println("thread no " + to_string(i) + ", start " + to_string(start) + ", end " + to_string(end));
        threads.push_back(thread(apiCallWorker, ref(pa), ref(httpCall), ref(lines), ref(pathVariables), ref(dataVariables), ref(varIndices),
                                 start, end, ref(callCounts[i]), ref(elappsedTimes[i]), ref(bufferedPrint)));
    }
    // This is for printing out call counts per second
    bool isDone = false;
    thread callCountWorker;
    if (!pa.isTestRun()) {
        callCountWorker = thread(latencyChecker, ref(callCounts), ref(elappsedTimes), ref(isDone), ref(bufferedPrint));
    }
    // joining worker threads
    for (auto& th : threads) {
        th.join();
    }
    isDone = true;
    if (callCountWorker.joinable()) {
        callCountWorker.join();
    }
    bufferedPrint.flush();
}

void run_easyapi(int argc, char*argv[]) {
    ParseArguments pa(argc, argv);
    if (pa.getMethod() == UNDEFINED) {
        cout << "Currently, support get, post, put, and delete calls." << endl;
        cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
        return;
    }

    vector<string> pathVariables = extractVariables(pa.getUrl());
    vector<string> dataVariables = extractVariables(pa.getData());

    if (pathVariables.empty() && dataVariables.empty()) {
        processSingleApiCall(pa);
    } else {
        int numPathVariables = getNumVariablesExceptFunctions(pathVariables);
        int numDataVariables = getNumVariablesExceptFunctions(dataVariables);
        // todo: add test to check existence of the file
        if ((numPathVariables > 0 || numDataVariables > 0) && pa.getDataFileName().empty()) {
            cout << "Variables are used, but data file is not given." << endl;
            cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
            return;
        }
        processMultipleApiCalls(pa, pathVariables, dataVariables);
    }
    return;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || 0 == strcmp(argv[1], "help")) {
        cout << "usage: easyapi get|post|put|delete url [data json] [optional parameters]" << endl << endl;
        cout << "- Simple one call cases" << endl;
        cout << "  # just give url and data." << endl;
        cout << "  easyapi get http://blabla.com/api/test" << endl;
        cout << "  easyapi post http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl;
        cout << "  easyapi put http://blabla.com/api/test/ 'any_string_data'" << endl;
        cout << "  easyapi delete post http://blabla.com/api/test/ '{\"key\":\"value\"}'" << endl << endl;
        cout << "- Making multiple calls using variables and data file" << endl;
        cout << "  # use ${var} in url path or data." << endl;
        cout << "  # in below example, ${var1} and ${var2} will be replaced with the values in data_file." << endl;
        cout << "  easyapi post http://blabla.com/api/${var1}/test '{\"key\":\"${var2}\"}' --data-file data_file" << endl << endl;
        cout << "  easyapi get http://blabla.com/api/${var1} --file data_file" << endl << endl;
        cout << "- About data file format" << endl;
        cout << "  # see this example" << endl;
        cout << "  var1, var2, var3   # first line should have list of variables. These variables also shuld exist in get-url path or data json of post call" << endl;
        cout << "  11, 22, 33         # var1, var2, var3 of get-url or post data-json will be replaced with these values." << endl;
        cout << "  ...                # number of calls should be same with number of these value lines." << endl;
        cout << "  ...                # If multiple threads are used, those will make calls by dividing given values" << endl << endl;
        cout << "- Using built-in rand function for a parameter." << endl;
        cout << "  # rand(x) will generate random integer between 0 and x (exclusive). rand(a, b) will generate a random integer between a and b." << endl;
        cout << "  easyapi get http://blabla.com/api/test/${=rand(10, 20)}" << endl;
        cout << "  easyapi put http://blabla.com/api/test/${=rand(100)}" << endl;
        cout << "  # can define --call-count (or -ct) to define number of calls when only random parameter is used." << endl;
        cout << "  easyapi get http://blabla.com/api/test/${=rand(10, 20)} --call-count 100" << endl;
        cout << "  # easyapi print out average api call latency. So, you can use easyapi for casual performance test with random parameters!" << endl << endl;
        cout << "- Optional parameters" << endl;
        cout << "  --file or -f" << "\t" << "Data file name containing list of path and data variables at the first line, and real datas in following lines." << endl;
        cout << "  --test-run or -tr" << "\t" << "Test run. Don't make real api calls. Instead showing command which will be used for real calls." << endl;
        cout << "  --num-threads or -nt" << "\t" << "Number of threads used for making multiple calls with multiple threads" << endl;
        cout << "  --output-format or -of" << "\t" <<"Output format. Default is json. Currently, if a string other than json is given, just printing out whatever received." << endl;
        cout << "  --time-out or -to" << "\t" << "Time out in milliseconds. Default is 0 which means no time out." << endl;
        cout << "  --call-count or -cc" << "\t" << "Define call counts. This is used when only random parameters are used. If template parameter and data file are given, this call-count is not used." << endl;
        return 0;
    }

    init_func_call();
    run_easyapi(argc, argv);

    return 0;
}
