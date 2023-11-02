//compile and run with either
// c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -DINLIB -ldl; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -DINLIB -DUSE_BOOST -ldl -I${BOOSTDIR}/include; ./a.out
// c++ -std=c++23 testStacktrace.cpp -g -DINMAIN -DINLIB -DUSE_BOOST -DBOOST_STACKTRACE_USE_BACKTRACE -ldl -I${BOOSTDIR}/include -I/data/user/innocent/gcc_src/libbacktrace/ /data/user/innocent/gcc_build/libbacktrace/.libs/libbacktrace.a; ./a.out
// or
// c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINLIB -fpic -shared -o liba.so -ldl;c++ -std=c++23 testStacktrace.cpp -lstdc++exp -g -DINMAIN -L. -la -Wl,-rpath=.; ./a.out
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


namespace __cxxabiv1
{
  extern "C" char*
  __cxa_demangle(const char* mangled_name, char* output_buffer, size_t* length,
                 int* status);
}


namespace
{
  char*
  demangle(const char* name)
  {
    int status;
    char* str = __cxxabiv1::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status == 0)
      return str;
    else
      {
        std::free(str);
        return nullptr;
      }
  }
}

#ifdef INLIB 
int nested_func2(int c)
{
#ifndef USE_BOOST
    std::cout << std::stacktrace::current() << '\n';
    // h[std::stacktrace::current()] = c;
   auto k = std::hash<std::stacktrace>()(std::stacktrace::current());
   Dl_info dlinfo;
   for (auto & entry : std::stacktrace::current() ) {
     he[entry] = k;
     auto fl = dladdr((const void*)(entry.native_handle()),&dlinfo);
     if (dlinfo.dli_sname) { 
       auto name = demangle(dlinfo.dli_sname);
       std::cout << name << ' ' << dlinfo.dli_fname <<'\n';
       std::free(name);
     }
     else std::cout << entry.description() << '\n';
   }
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
