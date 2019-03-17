#include "HttpCall.hpp"
#include <iostream>
#include <fstream>

using namespace std;

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

HttpCallResponse HttpCall::call(int key, HttpMethod method, const std::string& url, const std::string& data, int timeOut) const {
    CURL* curl = getCurl(key);
    if (curl == nullptr) {
        HttpCallResponse response;
        response.code = -1;
        response.response = "Failed to initialize libcurl.";
        return response;
    }
    if (!setOptions(curl, method, url, data, timeOut)) {
        HttpCallResponse response;
        response.code = -1;
        response.response = "Failed to set options.";
        return response;
    }

    string read_buf;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buf);
    CURLcode res = curl_easy_perform(curl);
    HttpCallResponse response;
    if(res != CURLE_OK) {
        response.code = -1;
        response.response = "Failed to make a http call.";
    } else {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        response.code = response_code;
        response.response = read_buf;
    }
    return response;
}

CURL* HttpCall::getCurl(int key) const {
    if (key < 0 || key >= curls.size()) {
        return nullptr;
    }
    return curls[key];
}

bool HttpCall::setOptions(CURL* curl, HttpMethod method, const string& url, const string& data, int timeOut) const {
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
        return true;
    }

    if (method == POST) {
        if (data.empty()) {
            return false;
        }

        //todo: will add other content type support later
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }

    if (method == PUT) {
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, NULL);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0L);
    } else if (method == DELETE) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    return true;
}

size_t HttpCall::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    string* read_buf = (string*)userdata;
    read_buf->append(ptr, size * nmemb);
    return size * nmemb;
}
