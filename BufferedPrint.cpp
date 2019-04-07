#include "BufferedPrint.hpp"
#include <iostream>
#include <algorithm>
#include <iterator>

using namespace std;

BufferedPrint::BufferedPrint(int _bufferSize) {
    buffer.reserve(_bufferSize);
}

void BufferedPrint::print(const string& output) {
    lock_guard<mutex> guard(print_mutex);
    if (buffer.size() + output.length() >= buffer.capacity()) {
        flush();
    }
    copy(output.begin(), output.end(), back_inserter(buffer));
}

void BufferedPrint::println(const string& output) {
    print(output + "\n");
}

void BufferedPrint::flush() {
    copy(buffer.begin(), buffer.end(), ostream_iterator<char>(cout, ""));
    buffer.clear();
}
