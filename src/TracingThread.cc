#include "mallocProfiler.h"

#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstdlib>

namespace {

  std::atomic<bool> run=true;

  void tracer() {
    using namespace std::chrono_literals;
    std::ostringstream ss;
    ss << "memstat_" << getpid() << ".mdr";
    std::ofstream out (ss.str());
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
    std::this_thread::sleep_for(std::chrono::seconds(30*60));
    std::ostringstream ss;
    ss << "memdump_" << getpid() << ".mdr";
    std::ofstream out (ss.str());
    mallocProfiler::dump(out);
    out.close();
    abort();
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
