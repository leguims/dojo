#
# Coding Dojo
#

cmake_minimum_required(VERSION 3.7)


project(dojo)

#
# Compilation settings
#

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
    # ensure a clean code
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion")
    set(CMAKE_C_FLAGS "-Wall -Wextra -Wconversion")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # brewed zeromq doesn't have cppzmq
    file(DOWNLOAD https://raw.githubusercontent.com/zeromq/cppzmq/v4.3.0/zmq.hpp includes/zmq.hpp EXPECTED_HASH SHA1=5916eb5dd3196ec97591e64c110789341b03b678)
    file(DOWNLOAD https://raw.githubusercontent.com/zeromq/cppzmq/v4.3.0/zmq_addon.hpp includes/zmq_addon.hpp EXPECTED_HASH SHA1=ffcfb809d128c8fd5a63ca078874f12cd4c7a517)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang doesn't have the handy gcc STL header file
        configure_file(stdc++.h.in includes/bits/stdc++.h COPYONLY @ONLY)
    endif()
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/includes)
endif()

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")


enable_testing()

#
# Executables
#

# main program
add_executable(pong
    src/pong.cpp)
target_link_libraries(pong
    PRIVATE ncurses zmq boost_program_options)

# test/demo
add_executable(logsrv examples/push-pull/logsrv.c)
target_link_libraries(logsrv PRIVATE zmq)

add_executable(logsrv2 examples/push-pull/logsrv2.c)
target_link_libraries(logsrv2 PRIVATE czmq)

add_executable(logsrv3 examples/push-pull/logsrv3.cpp)
target_link_libraries(logsrv3 PRIVATE zmq)

add_executable(demo examples/ncurses/demo.cpp)
target_link_libraries(demo PRIVATE ncurses)
