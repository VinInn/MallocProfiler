// compile wit
//  c++ -O3 -pthread -fPIC -shared -std=c++23 plugins/mallocProfiler.cc -lstdc++exp -o mallocProfiler.so -ldl -Iinclude

#include "mallocProfiler.h"
#include "utilities.h"

#include <cstdint>
#include <dlfcn.h>
#include <unistd.h>

#include<cassert>
#include <unordered_map>
#include<map>
#include<array>
#include<vector>
#include<algorithm>
#include <memory>

#include <mutex>
#include <thread>

#include <cstring>


#include <iostream>
#include <string>
#ifdef USE_BOOST
#include <boost/stacktrace.hpp>
using stacktrace = boost::stacktrace::stacktrace;
#else
#include <stacktrace>
using stacktrace = std::stacktrace;
#endif


namespace mallocProfiler {

  struct AtomicStat {
    std::atomic<double> mtot = 0;
    std::atomic<uint64_t> mlive = 0;
    std::atomic<uint64_t> mmax=0;
    std::atomic<uint64_t> ntot=0;

    void add(std::size_t size) {
       mtot += double(size);
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

}

namespace {

  using namespace  mallocProfiler;

  typedef std::thread Thread;
  typedef std::vector<std::thread> ThreadGroup;
  typedef std::mutex Mutex;
  typedef std::unique_lock<std::mutex> Lock;

#ifdef MALLOC_PROFILER_OFF
  bool defaultActive = false;
#else
  bool defaultActive = true;
#endif
  bool globalActive = false;
  bool beVerbose = false;
  bool doFinalThreadDump = false;
  bool doFinalDump = true;
  bool doRemangling = true;
  bool no0dump = false;

  Mutex globalLock;

  thread_local bool inMalloc = false;

  std::array<std::atomic<uint64_t>,64> memTotHist;
  std::array<std::atomic<uint64_t>,64> memLiveHist;
  static_assert(sizeof(std::array<std::atomic<uint64_t>,64>)==sizeof(Hist),"atomic size not the same");

  bool defaultRemangle(std::string & name) {
    const std::string doTrucate[] = {"__cxx11::basic_regex","TFormula","TClass","TCling","cling::","clang","llvm::","boost::spirit"};
    const std::string fakeSym[] = {"regex","TFormula","TClass","TCling","cling","clang","llvm","spirit"}; 
    // remove signature
    removeSignature(name);
    // remove alloctor
    removeTemplate(name,"std::allocator");
    // truncate if spirit, cling, Clang, std::regex etc
    int i=0;
    for (auto const & s : doTrucate) {
      if (name.find(s)!=std::string::npos) { name = fakeSym[i]; return true;}
      ++i;
    }
    return false;
  }

  remangleType remangler = defaultRemangle;

  bool remangle(std::string & name) {
     return remangler ? remangler(name) : false;
  } 

  std::string print_stacktrace(stacktrace const & st) {
     std::string trace;
     // reverse stack trace to fit flamegraph tool
     for (auto p=st.rbegin(); p!=st.rend(); ++p ) {
      auto entry = *p;
#ifdef USE_BOOST
      std::string name = entry.name();
#else
      std::string name = entry.description();
#endif
      bool truncate = false;
      if (doRemangling) truncate = remangle(name);
      if (!name.empty()) trace += name + ';';
      if (truncate) break;
     }
     // remove last semicolumn
     if (!trace.empty()) trace.erase(trace.size()-1,1);
     return trace;
  }


  stacktrace get_stacktrace() {
#ifdef USE_BOOST
    return boost::stacktrace::stacktrace();
#else
    return std::stacktrace::current();
#endif
  }


  std::size_t threshold = 128; // 1024;
  bool invertThreshold = false;

  AtomicStat globalStat;

struct  Me {

  using One = Stat;
#ifdef USE_BOOST
   using TraceMap = std::map<stacktrace,One>;
#else
   using TraceMap = std::unordered_map<stacktrace,One>; // need fix in hash
#endif
   using TraceVector = std::vector<std::pair<stacktrace,One>>;
   using Us = std::vector<std::unique_ptr<Me>>;



  Me() {
//    setenv("LD_PRELOAD","", true);
//    if (beVerbose)std::cout << "Recoding structure constructed in a thread " << getpid() << std::endl;
  }

  ~Me() {
    doRecording = false;
    if (beVerbose) std::cout << "MemStat Summary for " << getpid() << ':' << index << " : "  << stat.ntot << ' ' << stat.mtot << ' ' << stat.mlive << ' ' << stat.mmax <<' ' << memMap.size() <<' ' << calls.size() << std::endl;
    if (doFinalThreadDump) dump(std::cout, '$', SortBy::max);
  }

  void add(void * p, std::size_t size) {
    Lock guard(lock);
    // if we are here better doRecording be true
    assert(doRecording);
    doRecording = false;
    stat.add(size);
    bool cond = invertThreshold ? size > threshold : size < threshold;
    auto & e = cond ? smallAllocations : calls[get_stacktrace()];
    memMap[p] = std::make_pair(size, &e);
    e.add(size);
    globalStat.add(size);
    auto bin = 64 - __builtin_clzll(size);
    ++memTotHist[bin];
    ++memLiveHist[bin];
    doRecording = true;
  }

  bool sub(void * p)  {
    if (!p) return true;
    Lock guard(lock);
    // std::cout << "f " << p << std::endl;
    if (auto search = memMap.find(p); search != memMap.end()) {
     auto size = search->second.first;
     stat.sub(size);
     search->second.second->sub(size);
     globalStat.sub(size);
     auto bin = 64 - __builtin_clzll(size);
     --memLiveHist[bin];
     memMap.erase(p);
     return true;
    } 
    return false;
    // std::cout << "free not found " << p << std::endl; 
  
  }


  void mergeIn(TraceMap & allCalls, One & smallAlloc)  const {
     Lock guard(lock);
     smallAlloc.merge(smallAllocations);
     for ( auto const & e : calls) allCalls[e.first].merge(e.second);
  }


  static void loadTraceVector(TraceVector & v, SortBy sortMode, TraceMap const & calls, Mutex & alock) {
     using Elem = TraceVector::value_type;
     // auto comp = [](Elem const & a, Elem const & b) {return a.first < b.first;};
     auto comp = [](Elem const & a, Elem const & b) {return a.second.mmax < b.second.mmax;};
     // if (sortMode == SortBy::live) comp = [](Elem const & a, Elem const & b) {return a.second.mlive < b.second.mlive;};
     v.reserve(calls.size());
     {
       Lock guard(alock);
       for ( auto const & e : calls) { v.emplace_back(e.first,e.second);  std::push_heap(v.begin(), v.end(),comp);}
     }
     std::sort_heap(v.begin(), v.end(),comp);
  }

  static std::ostream & dump(std::ostream & out, char sep, SortBy sortMode, TraceMap const & calls, One const & smallAlloc, Mutex & alock) {
     TraceVector  v;
     loadTraceVector(v, sortMode, calls, alock);
     for ( auto const & e : v)  { 
       if (no0dump && 0==e.second.mlive) continue;
       out << "_mpTrace_;" << print_stacktrace(e.first) << ' ' << sep << e.second.ntot << sep << e.second.mtot << sep << e.second.mlive << sep << e.second.mmax << '\n';
     }
     auto &  e = smallAlloc;
     out << "_mpTrace_;" << (invertThreshold  ? "Large" :  "Small")  << "Allocations " << sep << e.ntot << sep << e.mtot << sep << e.mlive << sep << e.mmax << '\n';
     return out;
  }

   std::ostream & dump(std::ostream & out, char sep, SortBy sortMode) const {
     return  dump(out, sep, sortMode, calls, smallAllocations, lock);
   }


   std::ostream & details(std::ostream & out, std::string const & from) {
     auto sep = ' ';
     TraceVector  v;
     loadTraceVector(v, SortBy::max, calls, lock);
     
     int  lsize = from.empty() ? std::stacktrace().current().size()-4 : 0;
     for ( auto const & e : v)  {
       if (no0dump && 0==e.second.mlive) continue;
       std::ostringstream sout;
       sout <<"Stat " << e.second.ntot << sep << e.second.mtot << sep << e.second.mlive << sep << e.second.mmax << " at\n";
       int n=0;
       int last = e.first.size() - lsize;
       for (auto const & entry : e.first) {
         sout << '#' << n++ << ' ';
#ifdef USE_BOOST
         std::string name = entry.name();
#else
         std::string name = entry.description();
#endif 
         sout << name << ' ' << entry.source_file() <<  ':' << entry.source_line() << '\n';
         if (!from.empty() && std::string::npos!=name.find(from)) { out << sout.str(); break; }
         if (n==last) break;
       }  // symbols
       if (from.empty())  out << sout.str();
     }  //stacktraces
     return out;
   }

  void clear() {
    Lock guard(lock);
    auto previous = doRecording;
    doRecording = false;
    Stat zero;
    smallAllocations = zero;
    stat = zero;
    memMap.clear();
    calls.clear();
    doRecording = previous;
  }

  mutable Mutex lock;
  std::unordered_map<void*,std::pair<uint64_t,One*>> memMap; // active memory blocks 
  TraceMap calls;  // stat by stacktrace
  One smallAllocations; 
  One stat;
  int64_t index=-1;
  bool doRecording = defaultActive;

  static Us & us() {
   static Us us;
   if (us.capacity()<1024) us.reserve(1024);
   return us;
  }

  static Me & me(); 


  static void globalSub(void * p);


  static std::ostream & globalDump(std::ostream & out, char sep, SortBy sortMode) {
    // merge all stacktrace
    if (us().empty()) return out;
    inMalloc = true; // just avoid it...
    {
    TraceMap calls;
    One smallAlloc;
    std::vector<Me*> all;
    all.reserve(2*us().size());
    {
      Lock guard(globalLock);
      for (auto & e : us() ) all.push_back(e.get());
    }
    for (auto e : all) e->mergeIn(calls,smallAlloc);
    Mutex alock; // no need to lock
    dump(out, sep, sortMode, calls, smallAlloc, alock);
    }
    inMalloc = false;
    return out;
  }


  static std::ostream & globalDetails(std::ostream & out, std::string const & from) {

    return out;
  }

};

  thread_local Me * tlme = nullptr; 

  Me & Me::me() {
    if (tlme)  return *tlme;
    auto l = std::make_unique<Me>();
    tlme = l.get();
    {
      Lock guard(globalLock);
      tlme->index=us().size();
      us().push_back(std::move(l));
    }
    return *tlme;
  }


  bool isActive() {
    return globalActive && (!inMalloc) &&
    (tlme ? tlme->doRecording : defaultActive);
  }


  void Me::globalSub(void * p)  {
    if (!p) return;
    std::vector<Me*> all;
    all.reserve(2*us().size());
    {
      Lock guard(globalLock);
      for (auto & e : us() ) all.push_back(e.get());
    }
    auto here = tlme;
    for (auto e : all) {
     if (here == e) continue;
     if (e->sub(p)) return;
    }
    if (beVerbose) std::cout << "free not found " << p << std::endl;
  }

  typedef void * (*mallocSym) (std::size_t);
  typedef void (*freeSym) (void*);
  typedef void * (*callocSym)(std::size_t, std::size_t); // also for aligned_alloc
  typedef void * (*reallocSym)(void *, std::size_t);

  typedef void * (*dlopenSym)(const char *, int);

  dlopenSym origDL = nullptr;
  mallocSym origM = nullptr;
  freeSym origF = nullptr;
  callocSym origA = nullptr;
  callocSym origC = nullptr;
  reallocSym origR = nullptr;


  struct Banner {
    Banner() {
      assert(!globalActive);
      inMalloc = true;
      // setenv("LD_PRELOAD","", true);
      printf("malloc wrapper loading\n");
      for (auto & e : memTotHist) e=0;
      for (auto & e : memLiveHist) e=0;
      ad = Me::us().size();
      fflush(stdout);
      globalActive = defaultActive;
      inMalloc = false;
    }
    ~Banner() {
      globalActive = false;
      inMalloc = true;

      std::cout << "MemStat Global Summary for " << getpid() << "/"  << Me::us().size() << ": " << globalStat.ntot << ' ' << globalStat.mtot << ' ' << globalStat.mlive << ' ' << globalStat.mmax << std::endl;

      std::cout << "MemTot Global Hist for " << getpid() << "/"  << Me::us().size() << ": ";
      for (auto & e : memTotHist) std::cout << e << ',';
      std::cout << std::endl;

      std::cout << "MemLive Global Hist for " << getpid() << "/"  << Me::us().size() << ": ";
      for (auto & e : memLiveHist) std::cout << e << ',';
      std::cout << std::endl;

      if (doFinalDump) Me::globalDump(std::cout, '$', SortBy::max);
      globalActive = false;
      inMalloc = true;  // make sure we are not called again..
    }
    int ad;
  };

  Banner banner;

}

extern "C" 
{


void *dlopen(const char *filename, int flags) {
  if (!origDL) origDL = (dlopenSym)dlsym(RTLD_NEXT,"dlopen");
  auto previous = globalActive;
  globalActive = false;
  auto p  = origDL(filename,flags);
  globalActive = previous;
  return p;
}


void *malloc(std::size_t size) {
  if (!origM) origM = (mallocSym)dlsym(RTLD_NEXT,"malloc");
  assert(origM);
  auto p  =origM(size); 
  if (isActive()) {
    inMalloc = true;
    Me::me().add(p, size);
    inMalloc = false;
  }
  return p;
}



void *aligned_alloc(std::size_t alignment, std::size_t size ) {
  if (!origA) origA = (callocSym)dlsym(RTLD_NEXT,"aligned_alloc");
  assert(origA);
  auto p  = origA(alignment, size);
  if (isActive()) {
    inMalloc = true;
    Me::me().add(p, size);
    inMalloc = false;
  }
  return p;
}


// obsolete interface
void *memalign(size_t alignment, size_t size) {
   return aligned_alloc(alignment, size);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
   *memptr = aligned_alloc(alignment, size);
   return 0;
}

  
void *calloc(std::size_t count, std::size_t size ) {
  if (!origC) origC = (callocSym)dlsym(RTLD_NEXT,"calloc");
  assert(origC);
  auto p  = origC(count, size);
  if (isActive()) {
    inMalloc = true;
    Me::me().add(p, count*size);
    inMalloc = false;
  }
  return p;
}

/* it triggers an infinite recursion in "_dl_update_slotinfo"
void *realloc(void *ptr, std::size_t size) {
  if (!origR) origR = (reallocSym)dlsym(RTLD_NEXT,"realloc");
  assert(origR);

  // it may call free/malloc for what we know
  auto previous = doRecording;
  doRecording = false;
  auto p  = origR(ptr,size);
  doRecording = previous;

  if (globalActive&&doRecording) {
    doRecording = false;
    if((!tlme) || !Me::me().sub(ptr)) Me::globalSub(ptr);
    Me::me().add(p, size);
    doRecording = true;
  }
  return p;
}
*/

bool doingUSI = false;

// problem when called by "_dl_update_slotinfo"
void free(void *ptr) {
  if(!origF) origF = (freeSym)dlsym(RTLD_NEXT,"free");
  assert(origF);
  if (!doingUSI && globalActive && !inMalloc) {
    inMalloc = true;
    if((!tlme) || (!Me::me().sub(ptr)) ) Me::globalSub(ptr);
    inMalloc = false;
  }
  origF(ptr);
}



// update_get_addr is garanteed not inlined
// https://github.com/lattera/glibc/blob/master/elf/dl-tls.c#L797
typedef struct link_map * (*USI2sym)(struct tls_index *ti, size_t gen);
USI2sym origUSI2 = nullptr;
struct link_map *
update_get_addr(struct tls_index *ti, size_t gen) {
  if(!origUSI2) origUSI2 = (USI2sym)dlsym(RTLD_NEXT,"update_get_addr");
  assert(origUSI2);
  doingUSI = true;
  bool previous = globalActive;
  globalActive = false;
  auto p =  origUSI2(ti, gen);
  globalActive = previous;
  doingUSI = false;
  return p;
}


//
// these functions seems to modify gcc backtrace info
//

typedef void  (*rfiSym)(const void *, struct object *,void *, void *);
rfiSym origRFI = nullptr;
void
__register_frame_info_bases (const void *begin, struct object *ob,
                 void *tbase, void *dbase)
{
 if(!origRFI)  origRFI = (rfiSym)dlsym(RTLD_NEXT,"__register_frame_info_bases");
 bool previous = globalActive;
 globalActive = false;
 origRFI(begin,ob,tbase,dbase);
 globalActive = previous;
}

typedef void (*voidSym)();
voidSym origRF = nullptr;
void
_ZN4llvm14RuntimeDyldELF16registerEHFramesEv()
{
 if(!origRF)  origRF = (voidSym)dlsym(RTLD_NEXT,"_ZN4llvm14RuntimeDyldELF16registerEHFramesEv");
 bool previous = globalActive;
 globalActive = false;
 origRF();
 globalActive = previous;
}


} // extern C




namespace mallocProfiler {

   bool loaded() {return true;}

   bool active(bool allThreads) { return allThreads ? globalActive : Me::me().doRecording;}
   bool enabled () { return globalActive;}

   void enable () { globalActive = true;}
   void disable () { globalActive = false;}

   void activate(bool allThreads) {
     auto previous = globalActive;
     globalActive = false;
     if (allThreads) {
       defaultActive = true;
       Lock guard(globalLock);
       for (auto & e : Me::us()) e->doRecording=true;
     } else {
      Me::me().doRecording = true;
    }
    globalActive = previous;
   }

   void deactivate(bool allThreads) {
     auto previous = globalActive;
     globalActive = false;
     if (allThreads) {
       defaultActive = false;
       Lock guard(globalLock);
       for (auto & e : Me::us()) e->doRecording=false;
     } else {
      Me::me().doRecording = false;
    }
    globalActive = previous;
   }

   void clear(bool allThreads) {
     auto previous = globalActive;
     globalActive = false;
     if (allThreads) {
        Stat zero;
        memcpy(&globalStat,&zero,sizeof(Stat));
        for (auto & e : memTotHist) e=0;
        for (auto & e : memLiveHist) e=0;
        Lock guard(globalLock);
        for (auto & e : Me::us()) e->clear();
      } else {
       if (tlme)  Me::me().clear();
     }
     globalActive = previous;
   }

   void setThreshold(std::size_t value, bool reverse){ threshold = value; invertThreshold = reverse;}
   std::size_t getThreshold() { return threshold;}


  void setRemangler(remangleType f){remangler = f;}
  remangleType getDefaultRemangler(){return defaultRemangle;}
  remangleType getCurrentRemangler(){return remangler;}


   void noFinalDump(){ doFinalDump=false;}

   void noZeroLiveDump(bool no) {no0dump=no;}


   void setVerbose(bool isVerbose) {beVerbose=true;}

    Stat summary(bool allThreads)  {
     if (!allThreads) return Me::me().stat;
     Stat ret;
     memcpy(&ret,&globalStat,sizeof(Stat));
     return ret;
    } 


   Hist memTotHistogram() { 
     Hist ret;
     memcpy(&ret,&memTotHist,sizeof(Hist));
     return ret;
   }
   Hist memLiveHistogram() {
     Hist ret;
     memcpy(&ret,&memLiveHist,sizeof(Hist));
     return ret;
   }


   std::ostream &  dump(std::ostream & out, char sep, SortBy mode, bool allThreads) {
      auto previous = Me::me().doRecording;
      Me::me().doRecording = false;
      if (allThreads) Me::globalDump(out,sep,mode);
      else if(tlme) Me::me().dump(out, sep, SortBy::max);
      Me::me().doRecording = previous;
      return out;
   }

   std::ostream & dumpDetails(std::ostream & out, std::string const & from, bool allThreads) {
      auto previous = Me::me().doRecording;
      Me::me().doRecording = false;
      if (allThreads) Me::globalDetails(out,from);
      else if(tlme) Me::me().details(out, from);
      Me::me().doRecording = previous;
      return out;
   }

}
