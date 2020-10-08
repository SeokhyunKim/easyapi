# Easyapi: easy-to-use CLI tool for bulk API operations
Command-line API calling tool capable of bulk API calls with multi-threads and casual API performance tests.

## Example use cases
You can see below cases by running 'easyapi' or 'easyapi -h' after installation.
### Single api calls
```
easyapi get http://blabla.com/api/test
easyapi post http://blabla.com/api/test/ '{"key":"value"}'
easyapi put http://blabla.com/api/test/ 'any_string_data'
easyapi delete post http://blabla.com/api/test/ '{"key":"value"}'
```
### Bulk api calls with simple templating
Use ${var} in url path or data.
In below example, ${var1} and ${var2} will be replaced with the values in data_file.
```
easyapi post http://blabla.com/api/${var1}/test '{"key":"${var2}"}' --data-file data_file
easyapi get http://blabla.com/api/${var1} --file data_file
```
#### About data file format
```
# see this example
var1, var2, var3   # first line should have list of variables. These variables also shuld exist in get-url path or data json of post call
11, 22, 33         # var1, var2, var3 of get-url or post data-json will be replaced with these values.
...                # number of calls should be same with number of these value lines.
...                # If multiple threads are used, those will make calls by dividing given values
```  

## How to install in Mac OS
Get cmake.
```
brew install cmake
```
Get nlohmann-json which is a nice json library used in easyapi.
```
brew install nlohmann-json
```
Making directory for building easyapi. Let's try release version.
```
mkdir release
cd release
```
Now, you're in release directory. Running cmake to make makefile and build. ;)
```
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
That's it. You shuld be able to see your easyapi file at easyapi/release directory. Ad mentioned above, try 'easyapi -h' or just 'easyapi' to see how to use it.
