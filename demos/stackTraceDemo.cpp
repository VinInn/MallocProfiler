// compile with c++ -g 
//
#include <stacktrace>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <iostream>


#ifndef INLINE
#define INLINE __attribute__ ((noinline))
#endif


INLINE
int nestedFunc2(int c);
INLINE
int nestedFunc(int c);
inline
int instrumentedFunc(int c);



int nestedFunc2(int c) {
  return instrumentedFunc(c+1);
}


int nestedFunc(int c) {
  return nestedFunc2(c+1);
}

int instrumentedFunc(int c) {
  int nb=0;
  auto st = std::stacktrace::current();
  for (auto p=st.rbegin(); p!=st.rend(); ++p)  std::cout << '#' << nb++ << ' ' << (void*)((*p).native_handle()) << ' ' << (*p).description() << ' ' << (*p).source_file() << ':' << (*p).source_line() << '\n';

  return c + nb;;
}

// INLINE
inline
int func(int c) {
  return nestedFunc(c+1);
}

int main(int argc, char **) {
  std::cout << func(argc) << std::endl;
  return 0;
}
