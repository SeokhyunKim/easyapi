#include "HttpCall.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

#define READ_BUFFER_SIZE    (1024*100)

struct WriteData {
    char buffer[READ_BUFFER_SIZE];
    int length;
    WriteData() : length(0) {
        buffer[0] = '\0';
    }
};

string toString(HttpMethod method) {
    switch(method) {
        case GET:
            return "get";
        case POST:
            return "post";
        case PUT:
            return "put";
        case DELETE:
            return "delete";
        default:
            return "undefined";
    }
    return "";
}

HttpMethod fromString(const string& str) {
    if (0 == str.compare("get")) {
        return GET;
    } else if (0 == str.compare("post")) {
        return POST;
    } else if (0 == str.compare("put")) {
        return PUT;
    } else if (0 == str.compare("delete")) {
        return DELETE;
    }
    return UNDEFINED;
}

HttpCall::HttpCall() {
    curl_global_init(CURL_GLOBAL_ALL);
}

HttpCall::~HttpCall() {
    for (auto curl : curls) {
        if (curl != nullptr) {
            curl_easy_cleanup(curl);
        }
    }
    curl_global_cleanup();
}

int HttpCall::createKey() {
    lock_guard<mutex> lock(curls_mutex);
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return -1;
    }
    curls.push_back(curl);
    return curls.size() - 1;
}

void HttpCall::deleteKey(int key) {
    lock_guard<mutex> lock(curls_mutex);
    if (curls.size() >= key || key < 0) {
        return;
    }
    CURL* curl = curls[key];
    if (curl == nullptr) {
       return;
    }
    curl_easy_cleanup(curl);
    curls[key] = nullptr;
}

HttpCallResponse HttpCall::call(int key, HttpMethod method, const vector<string>& headers,
                                const std::string& url, const std::string& data, int timeOut) const {
    CURL* curl = getCurl(key);
    if (curl == nullptr) {
        HttpCallResponse response;
        response.code = -1;
        response.response = "Failed to initialize libcurl.";
        return response;
    }
    setOptions(curl, method, headers, url, data, timeOut);

    WriteData writeData;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&writeData);
    CURLcode res = curl_easy_perform(curl);
    HttpCallResponse response;
    if(res != CURLE_OK) {
        response.code = -1;
        response.response = "Failed to make a http call. Curl error message: " + string(curl_easy_strerror(res)) + ".";
    } else {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        response.code = response_code;
        response.response = writeData.buffer;
    }
    return response;
}

CURL* HttpCall::getCurl(int key) const {
    if (key < 0 || key >= curls.size()) {
        return nullptr;
    }
    return curls[key];
}

void HttpCall::setOptions(CURL* curl, HttpMethod method, const vector<string>& customHeaders,
                          const string& url, const string& data, int timeOut) const {
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpCall::write_callback);
    /* activate when debugging
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);*/

    if (timeOut > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeOut);
    }

    if (method == GET) {
        return;
    }

    if (method == POST && !data.empty()) {
        //todo: will add other content type support later
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        for (auto& customHeader : customHeaders) {
            headers = curl_slist_append(headers, customHeader.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    } else if (method == PUT) {
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, NULL);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0L);
    } else if (method == DELETE) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
}

size_t HttpCall::write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  WriteData* pWriteData = (WriteData*)userp;
  char* dst = pWriteData->buffer + pWriteData->length;

  size_t sizeToWrite = READ_BUFFER_SIZE - 1 - pWriteData->length;
  if (realsize < sizeToWrite) {
      sizeToWrite = realsize;
  }

  memcpy(dst, contents, sizeToWrite);
  pWriteData->length += sizeToWrite;
  pWriteData->buffer[pWriteData->length] = '\0';

  return sizeToWrite;
}
