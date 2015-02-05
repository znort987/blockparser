
SHELL=/bin/sh
MAKEFLAGS=-j20

CPLUS = g++

INC =                           \
        -I.                     \
        -DNDEBUG                \
        -DBITCOIN               \
        -DWANT_DENSE            \

#-DCLAM                  \
#-DBITCOIN               \
#-DDARKCOIN              \
#-DJUMBUCKS              \
#-DLITECOIN              \
#-DPEERCOIN              \
#-DFEDORACOIN            \
#-DMYRIADCOIN            \
#-DUNOBTANIUM            \
#-DPROTOSHARES           \

COPT =                          \
        -g0                     \
        -O6                     \
        -m64                    \
        -Wall                   \
        -flto                   \
        -msse3                  \
        -Wextra                 \
        -Wformat                \
        -pedantic               \
        -std=c++0x              \
        -ffast-math             \
        -march=native           \
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

.objs/rawdump.o : cb/rawdump.cpp
	@echo c++ -- cb/rawdump.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c cb/rawdump.cpp -o .objs/rawdump.o
	@mv .objs/rawdump.d .deps

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

.objs/blake.o : h9/blake.c
	@echo 'cc ' -- h9/blake.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/blake.c -o .objs/blake.o
	@mv .objs/blake.d .deps

.objs/bmw.o : h9/bmw.c
	@echo 'cc ' -- h9/bmw.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/bmw.c -o .objs/bmw.o
	@mv .objs/bmw.d .deps

.objs/cubehash.o : h9/cubehash.c
	@echo 'cc ' -- h9/cubehash.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/cubehash.c -o .objs/cubehash.o
	@mv .objs/cubehash.d .deps

.objs/echo.o : h9/echo.c
	@echo 'cc ' -- h9/echo.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/echo.c -o .objs/echo.o
	@mv .objs/echo.d .deps

.objs/groestl.o : h9/groestl.c
	@echo 'cc ' -- h9/groestl.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/groestl.c -o .objs/groestl.o
	@mv .objs/groestl.d .deps

.objs/jh.o : h9/jh.c
	@echo 'cc ' -- h9/jh.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/jh.c -o .objs/jh.o
	@mv .objs/jh.d .deps

.objs/keccak.o : h9/keccak.c
	@echo 'cc ' -- h9/keccak.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/keccak.c -o .objs/keccak.o
	@mv .objs/keccak.d .deps

.objs/luffa.o : h9/luffa.c
	@echo 'cc ' -- h9/luffa.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/luffa.c -o .objs/luffa.o
	@mv .objs/luffa.d .deps

.objs/shavite.o : h9/shavite.c
	@echo 'cc ' -- h9/shavite.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/shavite.c -o .objs/shavite.o
	@mv .objs/shavite.d .deps

.objs/simd.o : h9/simd.c
	@echo 'cc ' -- h9/simd.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/simd.c -o .objs/simd.o
	@mv .objs/simd.d .deps

.objs/skein.o : h9/skein.c
	@echo 'cc ' -- h9/skein.c
	@mkdir -p .deps
	@mkdir -p .objs
	@${CC} -MD ${INC} ${COPT}  -w -c h9/skein.c -o .objs/skein.o
	@mv .objs/skein.d .deps

.objs/pbkdf2.o : scrypt/pbkdf2.cpp
	@echo c++ -- scrypt/pbkdf2.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c scrypt/pbkdf2.cpp -o .objs/pbkdf2.o
	@mv .objs/pbkdf2.d .deps

.objs/scrypt.o : scrypt/scrypt.cpp
	@echo c++ -- scrypt/scrypt.cpp
	@mkdir -p .deps
	@mkdir -p .objs
	@${CPLUS} -MD ${INC} ${COPT}  -c scrypt/scrypt.cpp -o .objs/scrypt.o
	@mv .objs/scrypt.d .deps

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
    .objs/closure.o         \
    .objs/dumpTX.o          \
    .objs/help.o            \
    .objs/pristine.o        \
    .objs/rawdump.o         \
    .objs/rewards.o         \
    .objs/simpleStats.o     \
    .objs/sql.o             \
    .objs/taint.o           \
    .objs/transactions.o    \
                            \
    .objs/blake.o           \
    .objs/bmw.o             \
    .objs/cubehash.o        \
    .objs/echo.o            \
    .objs/groestl.o         \
    .objs/jh.o              \
    .objs/keccak.o          \
    .objs/luffa.o           \
    .objs/shavite.o         \
    .objs/simd.o            \
    .objs/skein.o           \
                            \
    .objs/pbkdf2.o          \
    .objs/scrypt.o          \
                            \
    .objs/callback.o        \
    .objs/opcodes.o         \
    .objs/option.o          \
    .objs/parser.o          \
    .objs/rmd160.o          \
    .objs/sha256.o          \
    .objs/util.o            \

parser:${OBJS}
	@echo lnk -- parser 
	@${CPLUS} ${LOPT} ${COPT} -o parser ${OBJS} ${LIBS}

clean:
	-rm -r -f *.o *.i .objs .deps *.d parser

-include .deps/*

