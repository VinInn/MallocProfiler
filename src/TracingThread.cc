#include "mallocProfiler.h"

#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

namespace {

  std::atomic<bool> run=true;

  void tracer() {
    mallocProfiler::deactivate(mallocProfiler::currentThread);
    using namespace std::chrono_literals;
    std::ostringstream ss;
    ss << "memstat_" << getpid() << ".mdr";
    std::ofstream out (ss.str());
    out << "MemStat Global Summary time: ncalls memtot memlive maxlive" << std::endl; 
    int t=0;
    while(run)  {
      std::this_thread::sleep_for(10s); // std::chrono::seconds(10)
      t+=10;
      auto globalStat = mallocProfiler::summary(true); 
      out << "MemStat Global Summary " << t << ": "  << globalStat.ntot << ' ' << globalStat.mtot << ' ' << globalStat.mlive << ' ' << globalStat.mmax << std::endl;
   }
   out.close();
  }

 void dumper() {
    mallocProfiler::deactivate(mallocProfiler::currentThread);
    int n=0;
    while(run)  {
      std::this_thread::sleep_for(std::chrono::seconds(10*60));
      std::cerr << "\n\n>>>> DUMPER GOING TO START <<<<<\n\n" << std::endl;
      std::ostringstream ss;
      ss << "memdump_" << getpid() << '_' << n++ << ".mdr";
      std::ofstream out (ss.str());
      mallocProfiler::dump(out);
      out.close();
      std::cerr << "\n\n>>>> DUMPER DONE <<<<<\n\n" << std::endl;
    }
 }


  struct Tracer {
    Tracer() {
      if (!mallocProfiler::loaded()) return;
      mallocProfiler::noFinalDump();
      std::thread t(tracer);
      t.detach();
      std::thread d(dumper);
      d.detach();
    }

    ~Tracer() {
       run=false;
    }

  };

  Tracer theTracer;

}
