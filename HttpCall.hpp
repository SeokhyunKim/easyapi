#ifndef _EASYCURL_HTTPCALL_HPP_
#define _EASYCURL_HTTPCALL_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <curl/curl.h>


enum HttpMethod { GET, POST, PUT, DELETE, UNDEFINED };
std::string toString(HttpMethod method);
HttpMethod fromString(const std::string& str);

struct HttpCallResponse {
    long code;
    std::string response;
};


class HttpCall {
private:
    std::vector<CURL*> curls;
    std::mutex curls_mutex;
public:
    HttpCall();
    ~HttpCall();

    int createKey();
    void deleteKey(int key);

    HttpCallResponse call(int key, HttpMethod method, const std::vector<std::string>& headers,
                          const std::string& url, const std::string& data="", int timeOut=0) const;

private:
    CURL* getCurl(int key) const;
    bool setOptions(CURL* curl, HttpMethod method, const std::vector<std::string>& headers,
                    const std::string& url, const std::string& data="", int timeOut=0) const;

    static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
};

#endif
