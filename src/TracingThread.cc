#include "mallocProfiler.h"

#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

namespace {

  std::chrono::seconds summaryInterval(10);
  std::chrono::seconds dumpInterval(10*60);
}


namespace mallocProfiler {

  void setDumpingInterval(std::chrono::seconds const & summaryLap, std::chrono::seconds const & dumpLap) {
      summaryInterval = summaryLap;
      dumpInterval = dumpLap;
  }

}

namespace {

  std::atomic<bool> run=true;

  void tracer() {
    mallocProfiler::deactivate(mallocProfiler::currentThread);
    using namespace std::chrono_literals;
    std::ostringstream ss;
    ss << "memstat_" << getpid() << ".mdr";
    std::ofstream out (ss.str());
    out << "MemStat Global Summary time: ncalls memtot memlive maxlive" << std::endl; 
    std::chrono::seconds t(0);
    while(run)  {
      std::this_thread::sleep_for(summaryInterval); 
      t+=summaryInterval;
      auto globalStat = mallocProfiler::summary(true);
      auto memTotHist = mallocProfiler::memTotHistogram(); 
      auto memLiveHist = mallocProfiler::memLiveHistogram();
      out << "MemStat Global Summary " << t.count() << ": "  << globalStat.ntot << ' ' << globalStat.mtot << ' ' << globalStat.mlive << ' ' << globalStat.mmax << std::endl;
      out << "MemTot Global Hist ";
      for (auto & e : memTotHist) out << e << ',';
      out << "\nMemLive Global Hist ";
      for (auto & e : memLiveHist) out << e << ',';
      out << std::endl;
   }
   out.close();
  }

 void dumper() {
    mallocProfiler::deactivate(mallocProfiler::currentThread);
    int n=0;
    while(run)  {
      std::this_thread::sleep_for(dumpInterval);
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
      mallocProfiler::noZeroLiveDump(true);
#ifdef STAT_ONLY
      mallocProfiler::setThreshold(0,true);
#endif
      std::thread t(tracer);
      t.detach();
#ifndef STAT_ONLY
      std::thread d(dumper);
      d.detach();
#endif
    }

    ~Tracer() {
       run=false;
    }

  };

  Tracer theTracer;

}
