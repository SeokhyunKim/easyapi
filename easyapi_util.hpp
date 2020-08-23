#ifndef _EASYAPI_UTIL_HPP_
#define _EASYAPI_UTIL_HPP_

#include <vector>
#include <string>

std::vector<std::string> extractVariables(const std::string& templateStr);
std::vector<std::string> tokenizeCSVLine(std::string line, const std::string& delimiters);
std::vector<std::string> removeFunctions(const std::vector<std::string>& vec);
bool isSame(const std::vector<std::string>& vec1, const std::vector<std::string>& vec2);
int getNumVariablesExceptFunctions(const std::vector<std::string>& vec);
std::string trim(std::string const& str);

#endif
