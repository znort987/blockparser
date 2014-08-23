cmake_minimum_required (VERSION 2.8)
project (blockparser)

add_definitions(-DNDEBUG)
add_definitions(-DBITCOIN)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g0 -O3 -Wall -m64 -msse3 -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wformat -Wno-deprecated -Wformat-security -Wstrict-aliasing=2 -Wno-variadic-macros -Wno-unused-variable -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -funroll-loops -fstrict-aliasing -fomit-frame-pointer")

# cflags are same as c++ flags except c++11 flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${PROJECT_SOURCE_DIR})

add_executable (parser
    callback.cpp
    opcodes.cpp
    option.cpp
    parser.cpp
    rmd160.cpp
    sha256.cpp
    util.cpp
    cb/allBalances.cpp
    cb/dumpTX.cpp
    cb/pristine.cpp
    cb/simpleStats.cpp
    cb/taint.cpp
    cb/closure.cpp
    cb/help.cpp
    cb/rewards.cpp
    cb/sql.cpp
    cb/transactions.cpp
    h9/blake.c
    h9/bmw.c
    h9/cubehash.c
    h9/echo.c
    h9/groestl.c
    h9/jh.c
    h9/luffa.c
    h9/shavite.c
    h9/simd.c
    h9/skein.c
)

target_link_libraries (parser crypto dl)
