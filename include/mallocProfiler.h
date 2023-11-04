#pragma once

#include<iosfwd>
#include<atomic>
#include<algorithm>

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
       mmax = std::max(mmax,other.mlive); // not correct use global
       ntot += other.ntot;
    }

  };


  struct AtomicStat {
    std::atomic<double> mtot = 0;
    std::atomic<uint64_t> mlive = 0;
    std::atomic<uint64_t> mmax=0;
    std::atomic<uint64_t> ntot=0;

    void add(std::size_t size) {
       mtot += size;
       mlive +=size;
       uint64_t lval = mlive;
       uint64_t prev = mmax;
       while(prev < lval && !mmax.compare_exchange_weak(prev, lval)){}
       ntot +=1;
    }
    void sub(std::size_t size) {
     mlive -=size;
    }
  };


   static_assert(sizeof(AtomicStat)==sizeof(Stat),"atomic size not the same");

   constexpr bool allThreads=true;
   constexpr bool currentThread=false;

   bool loaded();

   bool active(bool allThreads=true);

   void activate(bool allThreads=true);
   void deactivate(bool allThreads=true);

   void setThreshold(std::size_t value);

   void setVerbose(bool isVerbose);

   Stat summary(bool allThreads=true);

   void noFinalDump();

   std::ostream &  dump(std::ostream &, char separator='$', SortBy mode=SortBy::none, bool allThreads=true);

}
