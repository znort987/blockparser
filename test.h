#ifndef __TEST_H__
    #define __TEST_H__

    #include <string>
    #include <vector>

    struct Test {

    private:

        static void runFilter(
            const char  *filter,
            int         &failCount,
            int         &testCount
        );

    public:

        Test();
        virtual ~Test() {}
        virtual int run() = 0;
        virtual bool isActive() { return true; }
        virtual const std::string &getName() const = 0;

        static void runAll(char *filters[] = 0);
        static void pushMsg(const char *msg, ...);

        static void check(
            bool        &ok,
            bool        cond,
            const char  *func,
            const char  *file,
            int         line,
            const char  *cStr,
            const char  *msg = 0,
            ...
        );
    };

    class SimpleTest:public Test {
    private:
        typedef int (*Func)();

        const std::string   name;
        Func                func;
        bool                active;

    public:
        SimpleTest(
            Func        _func,
            const char  *_name,
            bool        _active = true
        )   :   name(_name),
                func(_func),
                active(_active)
        {
        }

        virtual int run()                           { return (*func)(); }
        virtual bool isActive()                     { return active;    }
        virtual const std::string &getName() const  { return name;      }
    };

    #define TEST_DECLARE(x) static SimpleTest t_##x(x, #x);

    #define TEST_CHECK(var, cond, ...)  \
    do                                  \
    {                                   \
        Test::check(                    \
            (var),                      \
            (cond),                     \
            __func__,                   \
            __FILE__,                   \
            __LINE__,                   \
            #cond,                      \
            ##__VA_ARGS__               \
        );                              \
    }                                   \
    while(0)                            \

    #define TEST_CHECKS(var, cond)      \
        TEST_CHECK(var, cond, 0)

#endif // __TEST_H__
