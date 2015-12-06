
#include <regex>
#include <string>
#include <test.h>
#include <vector>
#include <stdio.h>
#include <timer.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>

static std::vector<Test*> *tests;
static std::vector<std::string> msgs;

struct TestOrdering {
    bool operator()(
        const Test *const &a,
        const Test *const &b
    ) {
        auto aName = a->getName().c_str();
        auto bName = b->getName().c_str();
        return (strcasecmp(aName, bName)<0);
    }
};

Test::Test() {
    if(0==tests) {
        tests = new std::vector<Test*>;
    }
    tests->push_back(this);
}

void Test::runFilter(
    const char *_filter,
    int        &failCount,
    int        &testCount
) {
    if(0==tests) {
        return;
    }

    auto negate = false;
    if(_filter && '!'==_filter[0]) {
        negate = true;
        ++_filter;
    }

    std::string filter = ".*";
    filter += (_filter ? _filter : "");
    filter += ".*";

    TestOrdering ordering;
    std::stable_sort(
        tests->begin(),
        tests->end(),
        ordering
    );

    size_t maxLen = 0;
    for(auto test:*tests) {
        auto name = test->getName();
        auto f = filter.c_str();
        auto n = name.c_str();

        std::regex regexp(f, std::regex_constants::icase);
        auto matches = std::regex_match(n, regexp);
        if(negate) {
            matches = !matches;
        }
        if(matches) {
            maxLen = std::max(maxLen, name.size());
        }
    }

    auto fakeIt = (0!=getenv("FAKE"));
    for(auto test:*tests) {
        auto name = test->getName();
        auto f = filter.c_str();
        auto n = name.c_str();

        std::regex regexp(f, std::regex_constants::icase);
        auto matches = std::regex_match(n, regexp);
        if(negate) {
            matches = !matches;
        }
        if(false==matches) {
            continue;
        }

        printf("    %s ", name.c_str());

        auto nameSize = name.size();
        auto nbDots = (5 + maxLen) - nameSize;
        for(size_t j=0; j<nbDots; ++j) {
            putchar('.');
        }
        fflush(stdout);

        msgs.clear();

            if(false==test->isActive()) {
                printf(" %s ", "SKIP <----");
            } else {

                auto start = Timer::nanos();

                    auto status = fakeIt ? 0 : test->run();
                    if(0!=status && 2!=status) {
                        ++failCount;
                    }
                    ++testCount;

                auto now = Timer::nanos();
                auto elapsed = (now - start)*1e-9;
                printf(
                    " %s ",
                    status==0   ?   "pass"       :
                    status==1   ?   "FAIL <----" :
                    status==2   ?   "SKIP <----" :
                    "unknown"
                );
                printf("%7.3f secs ", elapsed);
            }

            for(size_t i=0; i<msgs.size(); ++i) {
                printf("%s ", msgs[i].c_str());
            }

        msgs.clear();
        printf("\n");
    }
}

void Test::runAll(
    char *filters[]
) {
    if(0==tests) {
        return;
    }

    auto start = Timer::nanos();
    auto nbTests = tests->size();
    printf("\nRegression tests: %d tests registered\n\n", (int)nbTests);

    auto failCount = 0;
    auto testCount = 0;
    if(filters==0 || filters[0]==0) {
        Test::runFilter(0, failCount, testCount);
    }
    while(*filters) {
        Test::runFilter(*(filters++), failCount, testCount);
    }

    printf("\nRegression tests:");
    if(0==failCount) {
        printf("all %d matched tests have passed.\n", testCount);
    } else {
        printf("%d/%d tests have failed.\n", failCount, testCount);
    }

    auto now = Timer::nanos();
    auto elapsed = (now-start)*1e-9;
    printf("Tests ran in %.3f secs.\n", elapsed);
    printf("\n");
}

void Test::pushMsg(
    const char *msg,
    ...
) {
    va_list list;
    va_start(list, msg);

        char *result = 0;
        auto r = vasprintf(&result, msg, list);
        if(r<0) {
            abort();
        }

        if(result) {
            msgs.push_back(result);
            free(result);
        }

    va_end(list);
}

void Test::check(
    bool        &ok,
    bool        cond,
    const char  *func,
    const char  *file,
    int         line,
    const char  *cStr,
    const char  *msg,
    ...
) {
    ok = ok && cond;

    if(false==cond) {
        fprintf(
            stderr,
            "\n"
            "\n"
            "In file           %s\n"
            "At line           %d\n"
            "In function       %s\n"
            "Condition failed: %s\n"
            "\n",
            file,
            line,
            func,
            cStr
        );

        if(msg) {
            va_list list;
            va_start(list, msg);
                vfprintf(stderr, msg, list);
            va_end(list);
        }

        fflush(stderr);
        fflush(stdout);
        assert(0);
    }
}

