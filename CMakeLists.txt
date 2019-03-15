cmake_minimum_required (VERSION 3.1)
project (easyapi)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.4.0 REQUIRED)

include_directories(${CURL_INCLUDE_DIR})

add_subdirectory(eval_func)

add_library(httpcall HttpCall.cpp)
add_library(parse_arguments ParseArguments.cpp)
add_executable(easyapi easyapi.cpp)

target_link_libraries(easyapi PRIVATE ${CURL_LIBRARIES} nlohmann_json::nlohmann_json httpcall parse_arguments eval_func)
