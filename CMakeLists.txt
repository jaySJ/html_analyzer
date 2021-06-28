cmake_minimum_required (VERSION 3.8)

project (HtmlAnalyzer)
set(EXEC_NAME "HtmlAnalyzer")

# Use C++ 14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # Visual Studio flag for openmp
    set(OpenMP_CXX_FLAGS "/openmp")
else()
    # Others
    set(OpenMP_CXX_FLAGS "-fopenmp")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# For fetching data from URLs, we need support for HTTPS/SSL
# We will use Boost ASIO with OpenSSL for this  (could also have used libcurl + openssl) 
# but for maintainability keeping the number of external libraries to a minimum.

# Open SSL
# file() converts a native <path> into a cmake-style path with forward-slashes (/). 
# The input can be a single path or a system search path like $ENV{PATH}.
file(TO_CMAKE_PATH $ENV{OPENSSL_ROOT} OPENSSL_ROOT_DIR)
message(WARNING "root: ${OPENSSL_ROOT_DIR}")
find_package(OpenSSL REQUIRED)

message(WARNING "OpenSSL_VERSION: ${OPENSSL_VERSION}")
message(WARNING "OpenSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(WARNING "OpenSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

# Boost (for using ASIO with openssl, spirit, qi, regex, property_tree, etc.)
# Find Boost headers and some static libraries
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         ON)  # ignore debug libs 
set(Boost_USE_RELEASE_LIBS       ON)  # find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

SET(BOOST_MANDATORY_COMPONENTS system filesystem date_time regex locale)
find_package(Boost "1.75.0" COMPONENTS ${BOOST_MANDATORY_COMPONENTS} REQUIRED)

message(WARNING "Boost was found: ${Boost_FOUND}")
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(WARNING "boost include dirs: ${Boost_INCLUDE_DIRS}")
    add_executable (${EXEC_NAME} "HtmlParser.cpp" "GetUrlContent.cpp" "main.cpp")
    target_include_directories(${EXEC_NAME} PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
    if(WIN32)
        target_link_libraries(${EXEC_NAME} OpenSSL::SSL crypt32 libomp)
    else ()
        target_link_libraries(${EXEC_NAME} OpenSSL::SSL crypt32 libiomp5)
    endif()
    target_link_libraries(${EXEC_NAME} ${Boost_LIBRARIES} ${Boost_SYSTEM_LIBRARY})
endif()

message(ERROR "Build failed")
