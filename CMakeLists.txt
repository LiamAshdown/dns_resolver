cmake_minimum_required(VERSION 4.1)
project(dns_resolver)

set(BOOST_INCLUDE_LIBRARIES asio filesystem system thread program_options)
set(BOOST_ENABLE_CMAKE ON)

include(FetchContent)
FetchContent_Declare(
        Boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.92.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Boost)

set(CMAKE_CXX_STANDARD 23)

add_executable(dns_resolver main.cpp)
target_link_libraries(dns_resolver PRIVATE
        Boost::asio
        Boost::filesystem
        Boost::thread
        Boost::program_options
)