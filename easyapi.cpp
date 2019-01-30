#include <iostream>
#include <string>
#include <cstddef>
#include <fstream>
#include <thread>
#include <vector>
#include <algorithm>
#include "HttpCall.hpp"

using namespace std;

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
            cout << httpCall.post(key, url, data) << endl;
        } else if (argc <= 7) {
            if (argc < 7) {
                cout << "Not enough parameters for data json having a variable." << endl;
                return 0;
            }
            string jsonTemplate = argv[3];
            string variable = argv[4];
            string replacement = "${" + variable + "}";
            string fileName = argv[5];
            int numThreads = atoi(argv[6]);
            fstream variableData(fileName);
            string line;
            vector<string> lines; 
            while (getline(variableData, line)) {
                lines.push_back(line);
            }
            cout << "Number of data-file lines: " << lines.size() << endl;
            cout << "Number of threads: " << numThreads << endl << endl;

            long chunkSize = lines.size() / numThreads;
            auto worker = [=, &httpCall](const vector<string>& lines, int start, int end) {
                int key = httpCall.createKey();
                for (int i=start; i<end; ++i) {
                    string line = lines[i];
                    string varjsonTemplate = jsonTemplate;
                    size_t pos = varjsonTemplate.find(replacement);
                    if (pos == string::npos) {
                        break;
                    }
                    varjsonTemplate.replace(pos, replacement.size(), line);
                    cout << httpCall.post(key, url, varjsonTemplate) << endl;
                }
            };
            vector<thread> threads;
            for (int i=0; i<numThreads; ++i) {
                threads.push_back(thread(worker, lines, i*chunkSize, std::min((i+1)*chunkSize, (long)lines.size())));
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
