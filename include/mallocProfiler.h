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

   bool loaded();

   bool active(bool allThreads=true);
   bool enabled ();

   void enable ();
   void disable ();
   void activate(bool allThreads=true);
   void deactivate(bool allThreads=true);
   void clear(bool allThreads=true);

   void setThreshold(std::size_t value, bool reverse=false);
   std::size_t getThreshold();

   void setVerbose(bool isVerbose);

   Stat summary(bool allThreads=true);

   void noFinalDump();

   void noZeroLiveDump(bool);

   std::ostream &  dump(std::ostream &, char separator='$', SortBy mode=SortBy::none, bool allThreads=true);
   
   std::ostream &  dumpDetails(std::ostream &, std::string const & from, bool allThreads=true);

   void setDumpingInterval(std::chrono::seconds const & summaryLap, std::chrono::seconds const & dumpLap);


  void setRemangler(remangleType);
  remangleType getDefaultRemangler();
  remangleType getCurrentRemangler();

}
