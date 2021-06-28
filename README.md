# html_analyzer

# Html Analyzer

An openMP-enabled multi-threaded C++ porgram that makes GET requests to multiple URL and analyzes the returned HTML body. 

**USAGE**

> build_x64\Release\HtmlAnalyzer.exe Urls.txt 4

Input:
    1. Text file containing URLs for fetching content.
    2. Number of threads
Output:
```
   ID                                        URL                # Nodes    # Leaf Nodes     # Div Nodes
    1   https://www.ibm.com/docs/en/ztpf/2020?to                     33             19              1
    2              https://www.openssl.org/docs/                    159             46              4
    3   https://raw.githubusercontent.com/nTopol                      0              0              0
    4                     https://ntopology.com/                   1353            330            145
    5   https://cmake.org/cmake/help/latest/vari                    258             74             14
    6   https://en.cppreference.com/w/cpp/algori                  11103           1557           1147
    7                 https://www.stratasys.com/                   1245            353            126
    8   https://github.com/boostorg/beast/issues                   2538            653            294
    9   https://www.google.com/search?q=omp_get_                     11              2              0
   10                   https://venturebeat.com/                   2549            608            111
   11           https://www.microsoft.com/en-us/                   1315            380            125
   12     https://www.microsoft.com/en-us/?spl=4                   1315            380            125
   13                     https://www.apple.com/                   1181            403            101
   14                         https://google.com                     20              8              0
   15               https://www.apple.com/music/                   1704            509            180

Elapsed time 21.197 seconds
```

## Build steps for Windows with Visual Studio 2019 Community

### Pre-reqs (3rd party libs and tools)
1. Visual Studio 2019 Community (for VC 14.2 version compiler)
2. Install [Strawberry Perl (needed for building openssl)](https://strawberryperl.com/) and make sure it is in your PATH.
3. Install [NASM](https://www.nasm.us/) and make sure it is in your PATH.

#### [OpenSSL version 1.1.0](https://www.openssl.org/)
This code uses boost::asio library with openSSL to make GET requests. Only version 1.1.1 of openssl has been tested.  

1. Install Strawberry Perl
2. Install NASM
3. Make sure both Perl and NASM are on your %PATH%
4. Fire up a Visual Studio Developer Command Prompt with *administrative privileges* (make sure you use the 32-bit one if you are building 32-bit OpenSSL, or the 64-bit one if you are building 64-bit OpenSSL)
5. From the root of the OpenSSL source directory enter perl Configure VC-WIN32, if you want 32-bit OpenSSL or perl Configure VC-WIN64A if you want 64-bit OpenSSL
     ```perl Configure VC-WIN64A no-asm no-shared```
6. Enter `nmake`
7. (OPTIONAL - it is slow) Enter `nmake test`
8. Enter `nmake install`.  This last step will build install openssl static libraries and include files in C:\Program Files\openSSL.
9. In order to build Boost::Asio library, you will need to create a new environment variable - OPENSSL_ROOT. 
   `OPENSSL_ROOT=C:\Program Files\OpenSSL`
#### Boost version 1.75.0
1. To build boost follow instructions
2. Make sure that `%BOOST_ROOT%` env variable is set to the path to static libraries and include files

# Build the code for this repo
1. From a Visual Studio Developer command prompt, call vcvarsall.bat (located at `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"`).
2. Use CMake to build - `cmake -G "Visual Studio 16 2019" -A x64 -S . -B "build_x64"` to generate VS project and solution files with appropriate compiler/linker flags for 2019 (boost/opensll/OpenMP). 
3. CMakeLists.txt file has been designed with portability for Windows/Linux in mind, but this has not been verified in Linux.
4. Build release branch: `msbuild HtmlAnalyzer.sln -m /property:Configuration=Release`. Use `-m` flag  for multiple-processor compilation as some of the boost headers can make the build slow.

## Build artifacts
Debug and release builds are located in `build_x64` sub-directory and can be found under sub-directories Debug and Rlease if built with the proper config. 

# Pre-built artifact
- Pre-built binary HtmlAnalyzer.exe that can be run on Windows is [here](https://github.com/jaySJ/html_analyzer/releases/tag/0.0).
- Sample outputs for OpenMP and non-OpenMP versions are in the repo's root.

# Known issues/limitations
1. Link "https://raw.githubusercontent.com/nTopology/JIRA-Priority-Icons/master/LICENSE" returns plain text, and not an HTML. Browsers transform the plain text into html for viewing. So the code cannot be expected to find any HTML tags for this URL.
2. Regex on some VC compiler versions is not handling some JavaScript syntax properly and as such **undercounts some nodes**.
3. Parallelism is limited to parsing/analyzing the HTML and also basic in nature. No parallelism/async when fetching URL content.
5. No unit tests


