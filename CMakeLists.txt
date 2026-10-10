cmake_minimum_required(VERSION 4.1)
project(dns_resolver)

set(BOOST_INCLUDE_LIBRARIES asio filesystem system thread program_options beast)
set(BOOST_ENABLE_CMAKE ON)

include(FetchContent)
FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.xz
        URL_HASH SHA256=9bed76128d4e46755dbe818487788c6fceb6f72b378f4daa49b7e1e600d9088d
)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.15.0
)

FetchContent_MakeAvailable(Boost)
FetchContent_MakeAvailable(spdlog)

set(CMAKE_CXX_STANDARD 23)

add_executable(dns_resolver main.cpp
        server.h
        connection.h
        dns.h
        buffer-reader.h
        buffer-writer.h
        constants.h)
target_link_libraries(dns_resolver PRIVATE
        Boost::asio
        Boost::filesystem
        Boost::thread
        Boost::program_options
        Boost::beast
        spdlog::spdlog
)
