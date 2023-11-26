#pragma once

#include<iosfwd>
#include<atomic>
#include<algorithm>
#include<chrono>

namespace mallocProfiler {

   enum class SortBy {none, tot, live, max, ncalls};

  struct Stat {
    double mtot = 0;
    uint64_t mlive = 0;
    uint64_t mmax=0;
    uint64_t ntot=0;

    void add(std::size_t size) {
       mtot += size;
       mlive +=size;
       mmax = std::max(mmax,mlive);
       ntot +=1;
    }
    void sub(std::size_t size) {
     mlive -=size;
    }

    void merge(Stat const & other) {
       mtot += other.mtot;
       mlive += other.mlive; 
       mmax = std::max(mmax,other.mmax); // not correct use global
       ntot += other.ntot;
    }

  };

  typedef bool (*remangleType)(std::string & name);

   constexpr bool allThreads=true;
   constexpr bool currentThread=false;

   // return true if the profiler has been correcly preloaded
   bool loaded();

   // return true if the  profiler is active for the current thread or for all threads (depending on the argument)
   bool active(bool allThreads=true);
   // return true if the profiler is globally enabled
   bool enabled ();

   // enable the profiler (all instrumentation)
   void enable ();
   // disable the profiler (all instrumentation)
   void disable ();
   // activate the  profiler for the current thread or for all threads (depending on the argument)
   void activate(bool allThreads=true);
   // activate the  profiler for the current thread or for all threads (depending on the argument)
   void deactivate(bool allThreads=true);
   // clear all data for the current thread or for all threads (depending on the argument)
   void clear(bool allThreads=true);

   // set the threshold above which stacktraces are recorded (soes not affect global statistics)
   void setThreshold(std::size_t value, bool reverse=false);
   // return the current threshold
   std::size_t getThreshold();

   // print some "debug" info
   void setVerbose(bool isVerbose);
   // return the statistics  for the current thread or for all threads (depending on the argument)
   Stat summary(bool allThreads=true);

   // disable the dump at the end of the process 
   void noFinalDump();

   // do not report stacktraces for which memlive is zero
   void noZeroLiveDump(bool);

   // dump al lstacktraces separating the stat fileds with a custom character, sorting them (not implemented)
   std::ostream &  dump(std::ostream &, char separator='$', SortBy mode=SortBy::none, bool allThreads=true);
   
   // dump stacktraces with one symbol per line including source file name and line number, stop at current function or to the first occurent of string "from" in the sybol name
   std::ostream &  dumpDetails(std::ostream &, std::string const & from, bool allThreads=true);

   // set the dumping intervals in case the dumping threads are loaded
   void setDumpingInterval(std::chrono::seconds const & summaryLap, std::chrono::seconds const & dumpLap);

  // set a custom remangler callback
  void setRemangler(remangleType);
  // return the default remangler callback
  remangleType getDefaultRemangler();
  // return the current remangler callback
  remangleType getCurrentRemangler();

}
