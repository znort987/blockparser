
SHELL=/bin/sh
MAKEFLAGS=-j8

CPLUS = g++

INC =                           \
        -I.                     \
        -DNDEBUG                \
#        -DLITECOIN              \

COPT =                          \
        -g0                     \
        -O6                     \
        -m64                    \
        -Wall                   \
        -msse3                  \
        -Wextra                 \
        -Wformat                \
        -pedantic               \
        -std=c++0x              \
        -ffast-math             \
        -fno-check-new          \
        -funroll-loops          \
        -Wno-deprecated         \
        -fstrict-aliasing       \
        -Wformat-security       \
        -Wstrict-aliasing=2     \
        -Wno-variadic-macros    \
        -fomit-frame-pointer    \
        -Wno-unused-variable    \
        -Wno-unused-parameter   \

LOPT =                          \
    -s                          \

LIBS =                          \
    -lcrypto                    \
    -ldl                        \

all:parser

.objs/callback.o : callback.cpp
	@echo c++ -- callback.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c callback.cpp -o .objs/callback.o
	@mv .objs/callback.d .deps

.objs/allBalances.o : cb/allBalances.cpp
	@echo c++ -- cb/allBalances.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/allBalances.cpp -o .objs/allBalances.o
	@mv .objs/allBalances.d .deps

.objs/closure.o : cb/closure.cpp
	@echo c++ -- cb/closure.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/closure.cpp -o .objs/closure.o
	@mv .objs/closure.d .deps

.objs/dumpTX.o : cb/dumpTX.cpp
	@echo c++ -- cb/dumpTX.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/dumpTX.cpp -o .objs/dumpTX.o
	@mv .objs/dumpTX.d .deps

.objs/pristine.o : cb/pristine.cpp
	@echo c++ -- cb/pristine.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/pristine.cpp -o .objs/pristine.o
	@mv .objs/pristine.d .deps

.objs/help.o : cb/help.cpp
	@echo c++ -- cb/help.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/help.cpp -o .objs/help.o
	@mv .objs/help.d .deps

.objs/rewards.o : cb/rewards.cpp
	@echo c++ -- cb/rewards.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/rewards.cpp -o .objs/rewards.o
	@mv .objs/rewards.d .deps

.objs/simpleStats.o : cb/simpleStats.cpp
	@echo c++ -- cb/simpleStats.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/simpleStats.cpp -o .objs/simpleStats.o
	@mv .objs/simpleStats.d .deps

.objs/sql.o : cb/sql.cpp
	@echo c++ -- cb/sql.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/sql.cpp -o .objs/sql.o
	@mv .objs/sql.d .deps

.objs/taint.o : cb/taint.cpp
	@echo c++ -- cb/taint.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/taint.cpp -o .objs/taint.o
	@mv .objs/taint.d .deps

.objs/transactions.o : cb/transactions.cpp
	@echo c++ -- cb/transactions.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/transactions.cpp -o .objs/transactions.o
	@mv .objs/transactions.d .deps

.objs/opcodes.o : opcodes.cpp
	@echo c++ -- opcodes.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c opcodes.cpp -o .objs/opcodes.o
	@mv .objs/opcodes.d .deps

.objs/option.o : option.cpp
	@echo c++ -- option.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c option.cpp -o .objs/option.o
	@mv .objs/option.d .deps

.objs/parser.o : parser.cpp
	@echo c++ -- parser.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c parser.cpp -o .objs/parser.o
	@mv .objs/parser.d .deps

.objs/rmd160.o : rmd160.cpp
	@echo c++ -- rmd160.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c rmd160.cpp -o .objs/rmd160.o
	@mv .objs/rmd160.d .deps

.objs/sha256.o : sha256.cpp
	@echo c++ -- sha256.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c sha256.cpp -o .objs/sha256.o
	@mv .objs/sha256.d .deps

.objs/util.o : util.cpp
	@echo c++ -- util.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c util.cpp -o .objs/util.o
	@mv .objs/util.d .deps

OBJS=                       \
    .objs/allBalances.o     \
    .objs/callback.o        \
    .objs/closure.o         \
    .objs/dumpTX.o          \
    .objs/help.o            \
    .objs/opcodes.o         \
    .objs/option.o          \
    .objs/parser.o          \
    .objs/pristine.o        \
    .objs/rewards.o         \
    .objs/rmd160.o          \
    .objs/sha256.o          \
    .objs/simpleStats.o     \
    .objs/sql.o             \
    .objs/taint.o           \
    .objs/transactions.o    \
    .objs/util.o            \

parser:${OBJS}
	@echo lnk -- parser 
	@${CPLUS} ${LOPT} ${COPT} -o parser ${OBJS} ${LIBS}

clean:
	-rm -r -f *.o *.i .objs .deps *.d parser

-include .deps/*

