// compile with
//  c++ -O3 -pthread -fPIC -shared -std=c++23 plugins/mallocProfiler.cc -lstdc++exp -o mallocProfiler.so -ldl -Iinclude

#include "mallocProfiler.h"

#include <cstdint>
#include <dlfcn.h>
#include <unistd.h>

#include<cassert>
#include <unordered_map>
#include<map>
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


namespace __cxxabiv1
{
  extern "C" char*
  __cxa_demangle(const char* mangled_name, char* output_buffer, size_t* length,
                 int* status);
}


namespace
{
  char*
  demangle(const char* name)
  {
    int status;
    char* str = __cxxabiv1::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status == 0)
      return str;
    else
      {
           std::free(str);
        return nullptr;
      }
  }
}


namespace {

  using namespace  mallocProfiler;

  typedef std::thread Thread;
  typedef std::vector<std::thread> ThreadGroup;
  typedef std::mutex Mutex;
  typedef std::unique_lock<std::mutex> Lock;
  // typedef std::condition_variable Condition;


  Mutex globalLock;

  std::string print_stacktrace(stacktrace const & st) {
     std::string trace;
     Dl_info dlinfo;
#ifdef USE_BOOST 
     for (auto & entry : st ) trace += entry.name() + ';';
#else
     // reverse stack trace to fit flamegraph tool
     for (auto p=st.rbegin(); p!=st.rend(); ++p ) {
      auto entry = *p;
      std::string name = entry.description();
      if (name.empty()) { 
        dladdr((const void*)(entry.native_handle()),&dlinfo);
        if(dlinfo.dli_sname) {
          auto cstr = demangle(dlinfo.dli_sname);
          if (cstr) name = cstr;
          std::free(cstr);
        }
      }
      trace += name + ';';
   }
#endif
     return trace;
  }


  stacktrace get_stacktrace() {
#ifdef USE_BOOST
    return boost::stacktrace::stacktrace();
#else
    return std::stacktrace::current();
#endif
  }


  bool globalActive = true;
  bool beVerbose = true;
  bool doFinalDump = true;

  thread_local bool doRecording = true;

  
  std::size_t threshold = 1024;

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
    if (doFinalDump) dump(std::cout, '$', SortBy::max);
  }

  void add(void * p, std::size_t size) {
    Lock guard(lock);
    stat.add(size);
    auto & e = size < threshold ? smallAllocations : calls[get_stacktrace()];
    memMap[p] = std::make_pair(size, &e);
    e.add(size);
    globalStat.add(size);
  }

  bool sub(void * p)  {
    if (!p) return true;
    Lock guard(lock);
    // std::cout << "f " << p << std::endl;
    if (auto search = memMap.find(p); search != memMap.end()) {
     stat.sub(search->second.first);
     search->second.second->sub(search->second.first);
     globalStat.sub(search->second.first);
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
     for ( auto const & e : v)  out << "_mpTrace_;" << print_stacktrace(e.first) << ' ' << sep << e.second.ntot << sep << e.second.mtot << sep << e.second.mlive << sep << e.second.mmax << '\n';
     auto &  e = smallAlloc;
     out << "_mpTrace_;SmallAllocations " << sep << e.ntot << sep << e.mtot << sep << e.mlive << sep << e.mmax << '\n';
     return out;
  }

   std::ostream & dump(std::ostream & out, char sep, SortBy sortMode) const {
     return  dump(out, sep, sortMode, calls, smallAllocations, lock);
   }



  mutable Mutex lock;
  std::unordered_map<void*,std::pair<uint64_t,One*>> memMap; // active memory blocks 
  TraceMap calls;  // stat by stacktrace
  One smallAllocations; 
  One stat;
  int64_t index=-1;

  static Us & us() {
   static Us us;
   if (us.capacity()<1024) us.reserve(1024);
   return us;
  }

  static Me & me() {
    thread_local Me * tlme = nullptr;
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


  static void globalSub(void * p)  {
    std::vector<Me*> all;
    all.reserve(2*us().size());
    {
      Lock guard(globalLock);
      for (auto & e : us() ) all.push_back(e.get());
    }
    auto here = &me();
    for (auto e : all) {
     if (here == e) continue;
     if (e->sub(p)) return;
    }
    if (beVerbose) std::cout << "free not found " << p << std::endl;
  }


  static std::ostream & globalDump(std::ostream & out, char sep, SortBy sortMode) {
    // merge all stacktrace
    auto previous = globalActive;
    globalActive = false;
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
    globalActive = previous;
    return out;
  }


};



  typedef void * (*mallocSym) (std::size_t);
  typedef void (*freeSym) (void*);
  typedef void * (*aligned_allocSym)( size_t alignment, size_t size );

  typedef void * (*dlopenSym)(const char *, int);

  dlopenSym origDL = nullptr;
  mallocSym origM = nullptr;
  freeSym origF = nullptr;
  aligned_allocSym origA = nullptr;


  struct Banner {
    Banner() {
      setenv("LD_PRELOAD","", true);
      printf("malloc wrapper loading\n");
      fflush(stdout);
    }
    ~Banner() {
      auto previous = globalActive;
      globalActive = false;
      std::cout << "MemStat Global Summary for " << getpid() << ": "  << globalStat.ntot << ' ' << globalStat.mtot << ' ' << globalStat.mlive << ' ' << globalStat.mmax << std::endl;
      globalActive = previous;
    }
  };

  Banner banner;

}

extern "C" 
{


void *dlopen(const char *filename, int flags) {
  if (!origDL) origDL = (dlopenSym)dlsym(RTLD_NEXT,"dlopen");
  auto previous = doRecording;
  doRecording = false;
  auto p  = origDL(filename,flags);
  doRecording = previous;
  return p;
}


void *malloc(std::size_t size) {
  if (!origM) origM = (mallocSym)dlsym(RTLD_NEXT,"malloc");
  assert(origM);
  auto p  = origM(size); 
  if (doRecording&&globalActive) {
    doRecording = false;
    Me::me().add(p, size);
    doRecording = true;
  }
  return p;
}

void *aligned_alloc( size_t alignment, size_t size ) {
  if (!origA) origA = (aligned_allocSym)dlsym(RTLD_NEXT,"aligned_alloc");
  assert(origA);
  auto p  = origA(alignment, size);
  if (doRecording&&globalActive) {
    doRecording = false;
    Me::me().add(p, size);
    doRecording = true;
  }
  return p;

}


void free(void *ptr) {
  if(!origF) origF = (freeSym)dlsym(RTLD_NEXT,"free");
  assert(origF);
  if (doRecording&&globalActive) {
    doRecording = false;
    if(!Me::me().sub(ptr)) Me::globalSub(ptr);
    doRecording = true;
  }
  origF(ptr);
}




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


/* does not work
typedef struct TInterpreter *  (*CIsym)(void* , const char**);
 CIsym oriCI = nullptr;
 struct TInterpreter *  CreateInterpreter(void* shlibHandle, const char* argv[]) {
  if(!oriCI)  oriCI = (CIsym)dlsym(RTLD_NEXT,"CreateInterpreter");
  bool previous = globalActive;
  globalActive = false;
  printf("in CI wrapper\n");
  fflush(stdout);
  auto p =  (oriCI)(shlibHandle,argv);
  globalActive = previous;
  return p;
 }
*/


/*
// wrap TROOT::InitInterpreter
typedef void (*RIISym)(struct TROOT *);
RIISym oriII = nullptr;
void
_ZN5TROOT15InitInterpreterEv(struct TROOT * ob) {
 if(!oriII)  oriII = (RIISym)dlsym(RTLD_NEXT,"_ZN5TROOT15InitInterpreterEv");
 bool previous = globalActive;
 globalActive = false;
 printf("im II wrapper\n");
 fflush(stdout);
 oriII(ob);
 globalActive = previous;
}

typedef long * (*macroSym)(struct TROOT * ob,char const*, int*, bool);
macroSym oriMacro = nullptr;
long *
_ZN5TROOT5MacroEPKcPib(struct TROOT * ob,const char *filename, int *error, bool padUpdate) {
 if(!oriMacro)  oriMacro = (macroSym)dlsym(RTLD_NEXT,"_ZN5TROOT5MacroEPKcPib");
 bool previous = globalActive;
 globalActive = false;
 printf("im Macro wrapper\n");
 fflush(stdout);
 auto p = oriMacro(ob,filename,error,padUpdate);
 globalActive = previous;
 return p;
}

//wrap clang::DeclContext::LoadLexicalDeclsFromExternalStorage
namespace clang{struct DeclContext;}
typedef bool (*exsSym)(struct clang::DeclContext *);
exsSym oriEXS = nullptr;
bool 
_ZNK5clang11DeclContext35LoadLexicalDeclsFromExternalStorageEv(struct clang::DeclContext * ob) {
 if(!oriEXS)  oriEXS = (exsSym)dlsym(RTLD_NEXT,"_ZNK5clang11DeclContext35LoadLexicalDeclsFromExternalStorageEv");
 bool previous = globalActive;
 globalActive = false;
 printf("im LoadLexicalDeclsFromExternalStorage wrapper\n");
 fflush(stdout);
 auto p = oriEXS(ob);
 globalActive = previous;
 return p;
}

*/

/*
typedef void  (*dlinitSym)(struct link_map *, int, char **, char **);
dlinitSym oriDLI = nullptr;
void
_dl_init (struct link_map *main_map, int argc, char **argv, char **env) {
  if(!oriDLI)  oriDLI = (dlinitSym)dlsym(RTLD_NEXT,"_dl_init");
  bool previous = globalActive;
  globalActive = false;
  oriDLI(main_map,argc,argv,env);
  globalActive = previous;
}
*/




} // extern C




namespace mallocProfiler {

   bool loaded() {return true;}

   bool active(bool allThreads) { return allThreads ? globalActive : doRecording;}

   void activate(bool allThreads) {
     if (allThreads) {
       globalActive = true;
     } else {
      doRecording = true;
    }
   }
   
   void deactivate(bool allThreads) {
     if (allThreads) {
       globalActive = false;
     } else {
      doRecording = false;
    }
   }

   void setThreshold(std::size_t value){ threshold = value; }

   void noFinalDump(){ doFinalDump=false;}

   void setVerbose(bool isVerbose) {beVerbose=true;}

    Stat summary(bool allThreads)  {
     if (!allThreads) return Me::me().stat;
     Stat ret;
     memcpy(&ret,&globalStat,sizeof(Stat));
     return ret;
    } 

   std::ostream &  dump(std::ostream & out, char sep, SortBy mode, bool allThreads) {
      if (allThreads) return Me::globalDump(out,sep,mode);
      auto previous = doRecording;
      doRecording = false;
      Me::me().dump(out, sep, SortBy::max);
      doRecording = previous;
      return out;
   }

}
