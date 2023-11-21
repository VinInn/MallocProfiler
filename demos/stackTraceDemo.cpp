//
// c++ -std=c++23 stackTraceDemo.cpp -lstdc++exp -g -O2 
//
#include <stacktrace>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <iostream>

struct __glibcxx_backtrace_state;

extern "C"
{

__glibcxx_backtrace_state*
__glibcxx_backtrace_create_state(const char*, int,
                                 void(*)(void*, const char*, int),
                                 void*);

int
__glibcxx_backtrace_pcinfo(__glibcxx_backtrace_state*, __UINTPTR_TYPE__,
                           int (*)(void*, __UINTPTR_TYPE__,
                                   const char*, int, const char*),
                           void(*)(void*, const char*, int),
                           void*);
}

namespace {
 void
  err_handler(void*, const char*, int)
  { 
    std::cout << "ERROR??" << std::endl;
  }

  __glibcxx_backtrace_state*
  init()
  {
    static __glibcxx_backtrace_state* state
      = __glibcxx_backtrace_create_state(nullptr, 1, err_handler, nullptr);
    return state;
  }
}

void printFull(uint64_t pc) {
  auto cb = [](void* self, uintptr_t, const char* filename, int lineno,
               const char* function) -> int {
   if (function!=nullptr) {
     std::cout << ">>  " << function << ' ' << filename << ':' << lineno << std::endl;
   }
   return function!=nullptr;
  };
  const auto state = init();
  ::__glibcxx_backtrace_pcinfo(state, pc, +cb, err_handler, nullptr);
}


#ifndef INLINE
#define INLINE [[gnu::optimize("no-optimize-sibling-calls")]]
// other options
// ''
// inline
// __attribute__ ((noinline))
// _attribute__((optimize("no-optimize-sibling-calls")))
#endif


#ifndef INLINE1
#define INLINE1 INLINE
#endif

#ifndef INLINE2
#define INLINE2 INLINE
#endif

#ifndef INLINE3
#define INLINE3 INLINE
#endif

#ifndef INLINE4
#define INLINE4 INLINE
#endif

INLINE4
int instrumentedFunc(int c) {
  int nb=0;
  auto st = std::stacktrace::current();
  for (auto p=st.rbegin(); p!=st.rend(); ++p)  {
     std::cout << '#' << nb++ << ' ' << (void*)((*p).native_handle()) << ' ' << (*p).description() << ' ' << (*p).source_file() << ':' << (*p).source_line() << '\n';
     printFull((*p).native_handle());
  }
  return c + nb;;
}

INLINE3
int nestedFunc2(int c) {
  return instrumentedFunc(c+1);
}

INLINE2
int nestedFunc(int c) {
  return nestedFunc2(c+1);
}


INLINE1
int func(int c) {
  return nestedFunc(c+1);
}

int main(int argc, char **) {
  std::cout << func(argc) << std::endl;
  return 0;
}
