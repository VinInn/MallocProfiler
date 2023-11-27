#include "../include/mallocProfiler.h"
#include<cstdio>
#include<iostream>

namespace {

  struct Configure {

     Configure();
     ~Configure();
  };


  Configure::Configure() {
    printf("custom configuration loading\n");
    std::cout.flush();
    using namespace mallocProfiler;

    // a trivial remangler (does nothing) 
    auto cb = [](std::string & s) -> bool { 
       return false; // do not truncate
    };
    setRemangler(+cb);
    // set threshold to zero
    setThreshold(0);
    // inhibit the final dump
    noFinalDump();
    // activate and enable the profiler
    activate(allThreads);
    enable();
  }


  Configure::~Configure() {

  }

  Configure configurer;

}
