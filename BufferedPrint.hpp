#ifndef _EASYAPI_BUFFEREDPRINT_HPP_
#define _EASYAPI_BUFFEREDPRINT_HPP_

#include <mutex>
#include <memory>
#include <vector>
#include <string>

class BufferedPrint {
public:
    BufferedPrint(int _bufferSize);

    void print(const std::string& output);
    void println(const std::string& output);

    void flush();

private:
    std::mutex print_mutex;
    std::vector<char> buffer;
};


#endif

