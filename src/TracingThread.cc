#include "mallocProfiler.h"

#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include "unistd.h"

namespace {

  std::atomic<bool> run=true;;

  void tracer() {
    using namespace std::chrono_literals;
    std::ostringstream ss;
    ss << "memstat_" << getpid() << ".mdr";
    std::ofstream out (ss.str());
    int t=0;
    while(run)  {
      std::this_thread::sleep_for(10000ms); // std::chrono::seconds(10)
      t+=10;
      auto globalStat = mallocProfiler::summary(true); 
      out << "MemStat Global Summary " << t << ": "  << globalStat.ntot << ' ' << globalStat.mtot << ' ' << globalStat.mlive << ' ' << globalStat.mmax << std::endl;
   }
   out.close();
  }


  struct Tracer {
    Tracer() {
      std::thread t(tracer);
      t.detach();
    }

    ~Tracer() {
       run=false;
    }

  };

  Tracer theTracer;

}
