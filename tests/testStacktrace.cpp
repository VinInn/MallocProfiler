//compile and run with either
// c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -DINLIB; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -DINLIB -DUSE_BOOST -ldl -I${BOOSTDIR}/include; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -DINLIB -DUSE_BOOST -DBOOST_STACKTRACE_USE_BACKTRACE -ldl -I${BOOSTDIR}/include -I/data/user/innocent/gcc_src/libbacktrace/ /data/user/innocent/gcc_build/libbacktrace/.libs/libbacktrace.a; ./a.out
// or
// c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINLIB -fpic -shared -o liba.so;c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINLIB -fpic -shared -o liba.so -ldl -I${BOOSTDIR}/include -DUSE_BOOST; c++ -std=c++23 -DUSE_BOOST testStacktrace.cpp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINLIB -fpic -shared -o liba.so -ldl -DUSE_BOOST -I${BOOSTDIR}/include -DBOOST_STACKTRACE_USE_BACKTRACE -I/data/user/innocent/gcc_src/libbacktrace/ /data/user/innocent/gcc_build/libbacktrace/.libs/libbacktrace.a; c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
//
//
#include <iostream>
#ifndef USE_BOOST
#include <stacktrace>
#else
#include <boost/stacktrace.hpp>
#endif

#ifdef INLIB 
int nested_func2(int c)
{
#ifndef USE_BOOST
    std::cout << std::stacktrace::current() << '\n';
#else
    std::cout << boost::stacktrace::stacktrace()  << '\n';
#endif
    return c + 1;
}
int nested_func(int c)
{
    return nested_func2(c + 1);
}
#else
int nested_func(int c);
#endif
#ifdef INMAIN
int func(int b)
{
    return nested_func(b + 1);
}
 
int main()
{
    std::cout << func(777) << std::endl;
   return 0;
}
#endif
