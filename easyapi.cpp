#include <iostream>
#include <string>
#include <cstring>
#include <cstddef>
#include <cmath>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <exception>
#include "HttpCall.hpp"
#include "ParseArguments.hpp"
#include <nlohmann/json.hpp>
#include "easyapi_util.hpp"
#include "BufferedPrint.hpp"
#include "eval_func/eval_func.h"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

string getHttpCallResponse(HttpCallResponse& response, const string& outputFormat) {
    if (response.code == 200l && 0 == outputFormat.compare("json")) {
        json j = json::parse(response.response);
        return j.dump(4);
    } else if (response.code > 0l) {
        return "code: " + to_string(response.code) + ", response: " + response.response;
    }
    return response.response;
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
    HttpCallResponse response = httpCall.call(key, pa.getMethod(), pa.getUrl(), pa.getData(), pa.getTimeOut());
    cout << getHttpCallResponse(response, pa.getOutputFormat()) << endl;
}

// api call thread worker
void apiCallWorker(const ParseArguments& pa, HttpCall& httpCall,
                   const vector<string>& lines, const vector<string>& variables, int start, int end,
                   int& callCount, long& elappsedTime, BufferedPrint& bufferedPrint) {
    int key = httpCall.createKey();
    for (int i=start; i<end; ++i) {
        string line = lines[i];
        string pathTemplate = pa.getUrl();
        string varjsonTemplate = pa.getData();
        vector<string> tokens = tokenizeCSVLine(line, pa.getDelimiters());
        string variables_string;
        bool isDataFine = true;
        for (int i=0; i<variables.size(); ++i) {
            string replacement = "${" + variables[i] + "}";
            if (variables.size() > tokens.size()) {
                isDataFine = false;
                break;
            }
            string value = tokens[i];
            variables_string += value + " ";
            size_t pos1= pathTemplate.find(replacement);
            if (pos1 != string::npos) {
                pathTemplate.replace(pos1, replacement.size(), value);
            }
            size_t pos2 = varjsonTemplate.find(replacement);
            if (pos2 != string::npos) {
                varjsonTemplate.replace(pos2, replacement.size(), value);
            }
        }
        if (!isDataFine) {
            bufferedPrint.println("Found data issue. httpCallKey: " + to_string(key) + ", data line no: " + to_string(i) + ", line: " + line);
            continue;
        }
        string inputVars;
        if (variables_string.length() > 0) {
            inputVars = "Input variables: " + variables_string + "\n";
        }
        if (pa.isTestRun()) {
            bufferedPrint.println(inputVars + getTestCommand(pa.getMethod(), pathTemplate, varjsonTemplate));
        } else {
            auto start = high_resolution_clock::now();
            HttpCallResponse response = httpCall.call(key, pa.getMethod(), pathTemplate, varjsonTemplate, pa.getTimeOut());
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            elappsedTime += duration.count();
            bufferedPrint.println(inputVars + "Response:\n" + getHttpCallResponse(response, pa.getOutputFormat()));
        }
        callCount += 1;
    }
}

// api call thread worker
void apiCallWorker2(const ParseArguments& pa, HttpCall& httpCall, int numCalls,
                   int& callCount, long& elappsedTime, BufferedPrint& bufferedPrint) {
    int key = httpCall.createKey();
    string path = pa.getUrl();
    string data = pa.getData();
    while (numCalls-- > 0) {
        if (pa.isTestRun()) {
            bufferedPrint.println(getTestCommand(pa.getMethod(), path, data));
        } else {
            auto start = high_resolution_clock::now();
            HttpCallResponse response = httpCall.call(key, pa.getMethod(), path, data, pa.getTimeOut());
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            elappsedTime += duration.count();
            bufferedPrint.println("Response:\n" + getHttpCallResponse(response, pa.getOutputFormat()));
        }
        callCount += 1;
    }
}

// thread worker for printing out average api latency
void latencyChecker(const vector<int>& callCounts, const vector<long>& elapsedTimes, bool& isDone, BufferedPrint& bufferedPrint) {
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
        for_each(elapsedTimes.begin(), elapsedTimes.end(), [&totalTime](long t) { totalTime += t; });
        int totalCount = 0;
        for_each(callCounts.begin(), callCounts.end(), [&totalCount](int c) { totalCount += c; });
        if (totalTime <= 0l || totalCount <= 0l) {
            this_thread::sleep_for(milliseconds(100));
            continue;
        }
        double avgTime = (double)totalTime / totalCount;
        auto elapsed = duration_cast<milliseconds>(system_clock::now() - startTime).count();
        bufferedPrint.println("Average time for api call: " + to_string(avgTime) +
                              " ms, Total call count: " + to_string(totalCount) +
                              ", Total elapsed time: " + to_string(elapsed) +
                              " ms");
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
    vector<string> firstLineVariables = tokenizeCSVLine(firstLine, pa.getDelimiters());
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
    if (!pa.isForceRun()) {
        cout << "The first line of the data file: " << firstLine << endl;
        cout << endl << "Everything is looking good? (y/n) ";
        char isProceed;
        cin >> isProceed;
        if (isProceed != 'y' && isProceed != 'Y') {
            return;
        }
    }

    // divide chuncks by numThreads and launch workers
    long chunkSize = (long)ceil((double)lines.size() / pa.getNumThreads());
    if (chunkSize <= 0l) {
        cout << "Too many threads. Number of input lines: " << lines.size() << ", Thread number: " << pa.getNumThreads() << endl;
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
        long end = std::min((i+1)*chunkSize, (long)lines.size());
        bufferedPrint.println("thread no " + to_string(i) + ", start " + to_string(start) + ", end " + to_string(end));
        threads.push_back(thread(apiCallWorker, ref(pa), ref(httpCall), ref(lines), ref(firstLineVariables), start, end,
                                 ref(callCounts[i]), ref(elappsedTimes[i]), ref(bufferedPrint)));
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

void processMultipleApiCalls(const ParseArguments& pa) {
    HttpCall httpCall;

    if (pa.getNumApiCalls() < 1) {
        cout << "Number of api calls is less than 1. Aborting." << endl;
        return;
    }

    // print parameters before executing multiple threads for making lots of api calls.
    cout << "Number of api calls: " << pa.getNumApiCalls() << endl;
    cout << "Number of threads: " << pa.getNumThreads() << endl << endl;
    if (!pa.isForceRun()) {
        cout << "Everything is looking good? (y/n) ";
        char isProceed;
        cin >> isProceed;
        if (isProceed != 'y' && isProceed != 'Y') {
            return;
        }
    }

    // divide chuncks by numThreads and launch workers
    long callCountsPerThread = (long)ceil((double)pa.getNumApiCalls() / pa.getNumThreads());
    if (callCountsPerThread <= 0l) {
        cout << "Too many threads. Number of api calls: " << pa.getNumApiCalls() << ", Thread number: " << pa.getNumThreads() << endl;
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
        long start = i*callCountsPerThread;
        long end = std::min((i+1)*callCountsPerThread, (long)pa.getNumApiCalls());
        bufferedPrint.println("thread no " + to_string(i) + ", num api calls: " + to_string(end-start));
        threads.push_back(thread(apiCallWorker2, ref(pa), ref(httpCall), (int)(end - start + 1),
                                 ref(callCounts[i]), ref(elappsedTimes[i]), ref(bufferedPrint)));
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

    if (pathVariables.empty() && dataVariables.empty() && pa.getNumApiCalls() < 1) {
        processSingleApiCall(pa);
    } else {
        // todo: add test to check existence of the file
        if (pa.getDataFileName().empty() && pa.getNumApiCalls() < 1) {
            cout << "Variables are used, but data file is not given." << endl;
            cout << "To see usage, just type 'easyapi' or 'easyapi help'." << endl;
            return;
        }
        if (!pathVariables.empty()) {
            fstream variableData(pa.getDataFileName());
            processMultipleApiCalls(pa, pathVariables, dataVariables, variableData);
        } else {
            processMultipleApiCalls(pa);
        }
    }
    return;
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
        cout << "  --output-format or -ot" << "\t" <<"Output format. Default is json. Currently, if a string other than json is given, just printing out whatever received." << endl;
        cout << "  --time-out or -to" << "\t" << "Time out in milliseconds. Default is 0 which means no time out." << endl;
        cout << "  --delimiters or -d" << "\t" << "Defile a delimiter. Default is space and comma (\" ,\")." << endl;
        cout << "  --force-run or -fr" << "\t" << "Forced run. If this option is set, not asking to check input parameters. So, please be very cautious when using this option." << endl;
        cout << "  --num-api-calls or -nc" << "\t" << "The number of api calls. When data-file is not set and this is greater than zero, api calls will be repeated by this parameter." << endl;
        return 0;
    }

    try {
        run_easyapi(argc, argv);
    } catch (bad_alloc& e) {
        cout << "Allocation failed: " << e.what() << endl;
    } catch (...) {
        cout << "Opps what is this?" << endl;
    }

    return 0;
}
