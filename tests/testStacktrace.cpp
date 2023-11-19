//compile and run with either
// c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -DINLIB -ldl; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -DINLIB -DUSE_BOOST -ldl -I${BOOSTDIR}/include; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -DINLIB -DUSE_BOOST -DBOOST_STACKTRACE_USE_BACKTRACE -ldl -I${BOOSTDIR}/include -I/data/user/innocent/gcc_src/libbacktrace/ /data/user/innocent/gcc_build/libbacktrace/.libs/libbacktrace.a; ./a.out
// or
// c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINLIB -fpic -shared -o liba.so -ldl;c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
//  c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINLIB -fpic -shared -o liba.so -ldl -flto -O3; c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -L. -la -Wl,-rpath=. -flto -O3; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINLIB -fpic -shared -o liba.so -ldl -I${BOOSTDIR}/include -DUSE_BOOST; c++ -std=c++23 -DUSE_BOOST testStacktrace.cpp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINLIB -fpic -shared -o liba.so -ldl -DUSE_BOOST -I${BOOSTDIR}/include -DBOOST_STACKTRACE_USE_BACKTRACE -I/data/user/innocent/gcc_src/libbacktrace/ /data/user/innocent/gcc_build/libbacktrace/.libs/libbacktrace.a; c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
//
//
#include <iostream>
#ifndef USE_BOOST
#include <stacktrace>
#include <dlfcn.h>
#else
#include <boost/stacktrace.hpp>
#endif

#include<vector>
#include<map>
#include<unordered_map>

using V = std::vector<std::stacktrace>;
using M = std::map<std::stacktrace,int>;
using H = std::unordered_map<std::stacktrace,int>;
using HE = std::unordered_map<std::stacktrace_entry,int>;

V v;
M m;
H h;
HE he;



#ifdef INLIB 
inline
int nested_func2(int c)
{
#ifndef USE_BOOST
   std::cout << std::stacktrace::current() << '\n';
   auto k = std::hash<std::stacktrace>()(std::stacktrace::current());
   int n=0;
   auto st = std::stacktrace::current();
   int nf=0;
   for (auto p=st.begin(); p!=st.end(); ++p) nf++;
   int nb=0;
   for (auto p=st.rbegin(); p!=st.rend(); ++p)  std::cout << '#' << nb++ << ' ' << (void*)((*p).native_handle()) << ' ' << (*p).description() << ' ' << (*p).source_file() << ':' << (*p).source_line() << '\n';
   std::cout << "st size " << st.size() << ' ' << n << ' ' << nf << ' ' << nb << '\n' << std::endl;
#else
    std::cout << boost::stacktrace::stacktrace()  << '\n';
#endif
    return k ? c + 1 : 0;
}
__attribute__ ((noinline))
// inline
int nested_func(int c)
{
    asm volatile("");
    return nested_func2(c + 1);
}
#else
int nested_func(int c);
#endif
#ifdef INMAIN
// int func(int b) __attribute__((noinline));
inline
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
