// compile with c++ -g instrumentationDemo,cpp ../dummyMallocProfiler.so -o instrumentationDemo
// or compile with c++ -g instrumentationDemo,cpp ../dummyMallocProfiler.so -o instrumentationDemo -DRESERVE
//
#include "../include/mallocProfiler.h"
#include <unordered_map>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <iostream>

struct A {
  double a;
  int b;
};

using HM = std::unordered_map<int, A>;

int instrumentedFunc(int c) {

  // set the threshold ofor recording stacktraces to zero
  mallocProfiler::setThreshold(0);
  // enable the profiler
  mallocProfiler::enable();
  // activate the profiler for the current thread
  mallocProfiler::activate(mallocProfiler::currentThread);

  // create a hashmap
  HM hm;

#ifdef RESERVE
  // reserve space
  hm.reserve(100000);
#endif
  // populate the map;
  for (int i=0; i<100000; ++i) {
    // create a new entry 
    auto & a = hm[i*10];
    // fill the entry
    a.a = i/10.;
    a.b = i;
  }
  assert(100000==hm.size());
#ifndef DUMP_COMPACT
  //  dump details about stacktraces where malloc was called (stop stracktrace depth at current function)
  mallocProfiler::dumpDetails(std::cout, "", mallocProfiler::currentThread);
#else
  // or dump the compact form used for speedscope
  mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max, mallocProfiler::currentThread);
#endif
  //  deactivate the profiler
  mallocProfiler::deactivate(mallocProfiler::currentThread);
  // globaly disable it
  mallocProfiler::disable();

  // If what we want was just the profile we can quit here!
  if (c>0) std::quick_exit(0);

  return c + hm.size();
}

int nestedFunc2(int c) {
  return instrumentedFunc(c+1);
}

int nestedFunc(int c) {
  return nestedFunc2(c+1);
}

int func(int c) {
  return nestedFunc(c+1);
}

int main(int argc, char **) {


  return func(argc);

}
