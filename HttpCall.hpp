#ifndef _EASYCURL_HTTPCALL_HPP_
#define _EASYCURL_HTTPCALL_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <curl/curl.h>

class HttpCall {
private:
    std::vector<CURL*> curls;
    std::mutex curls_mutex;
public:
    HttpCall();
    ~HttpCall();

    int createKey();
    void deleteKey(int key);

    std::string get(int key, const std::string& url) const;
    std::string post(int key, const std::string& url, const std::string& json) const;

    static bool isFailed(const std::string& response);
    
private:
    CURL* getCurl(int key) const;

    static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
};

#endif
