#include "HttpCall.hpp"
#include <iostream>
#include <fstream>

using namespace std;

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

string HttpCall::get(int key, const string& url) const {
    CURL* curl = getCurl(key);
    if (curl == nullptr) {
        return "";
    }
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpCall::write_callback);
    string read_buf;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buf);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        cout << "Failed to get " + url << endl;
        cout << "Error msg: " << curl_easy_strerror(res) << endl;
        return "";
    }
    return read_buf;
}

string  HttpCall::post(int key, const string& url, const string& json) const {
    CURL* curl = getCurl(key);
    if (curl == nullptr) {
        return "";
    }
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpCall::write_callback);
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    string read_buf;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buf);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        cout << "Failed to post. url:  " << url << ", data: " << json << endl;
        cout << "Error msg: " << curl_easy_strerror(res) << endl;
        return "";
    }
    return read_buf;
}

bool HttpCall::isFailed(const string& response) {
    if (string::npos == response.find("Failed to ")) {
        return false;
    }
    return true;
}

CURL* HttpCall::getCurl(int key) const {
    if (key < 0 || key >= curls.size()) {
        return nullptr;
    }
    return curls[key];
}

size_t HttpCall::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    string* read_buf = (string*)userdata;
    read_buf->append(ptr, size * nmemb);
    return size * nmemb;
}
