#include "../include/mallocProfiler.h"

#include<vector>


double dummy = 0;


#include<cstdio>
#include<iostream>
#include<iomanip>
#include<cassert>

// from libs
void a(std::vector<int*>&v);


void allAlloc() {

 auto a = (int*)malloc(4*1024);
 auto b = (int*)calloc(4,1024);
 auto c = (int*)realloc(a,8*1024);
 auto d = (int*)aligned_alloc(64,4*1024);

 if (a==c) std::cout << "realloc same" << std::endl;

 mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max, false);

 assert(0==b[24]);
 assert(c!=d);
 assert(0==(((uint64_t)(d))&7));
 free(b);
 free(c);
 free(d);

 mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max, false);

};


template<typename T, int mode>
void go(int size) {

  std::vector<int> v;
  if (mode==1) v.reserve(size);

  for (int i=0; i<size; ++i)
    v.push_back(T(i));

  dummy +=v[5];

  
  mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max, false);

}


struct Ex{ std::size_t s;};

void testEx(std::vector<int*> & v, int n) {
   v.push_back(new int(4));
   if (v.size()>10) throw Ex{v.size()};
}


[[gnu::optimize("no-optimize-sibling-calls")]]
void recursive(std::vector<int*> & v, int n) {
  if (0==n) return;
  v.push_back(new int(4));
  recursive(v,n-1);
}

#include<iostream>
#include<chrono>
#include<thread>
using namespace std::chrono_literals;

int main() {

  printf("START\n");
  fflush(stdout);

  mallocProfiler::setDumpingInterval(1s,1min);
  mallocProfiler::setThreshold(0);

  std::cout << "profiler status " << std::boolalpha << mallocProfiler::active(mallocProfiler::allThreads) << ' ' << mallocProfiler::active(mallocProfiler::currentThread)
            << " threshold "  << mallocProfiler::getThreshold() << std::endl;


  allAlloc();
  std::cout << std::endl;

  go<int,0>(100);
  go<int,1>(1000);
  go<double,0>(1000);

  std::cout << std::endl;

  {
  std::vector<int*> v;
  recursive(v,4);  // tested with 257 as well

  std::cout << v.size() << std::endl;

  std::cout << "test exception" << std::endl;

  try {
     testEx(v,4);
     std::cout << v.size() << std::endl;
  }
  catch (Ex & ex) {
     std::cout << "catched " << ex.s << std::endl;
  }


  std::cout << "calling libs" << std::endl;
  a(v);
  mallocProfiler::dumpDetails(std::cout, "BBB", mallocProfiler::currentThread);

#ifdef SLEEP
  std::this_thread::sleep_for(3s);
#endif

  }

#ifdef SLEEP
  std::this_thread::sleep_for(2s);
#endif

  std::cout << "=== Global Profile ===" << std::endl;
  mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max,  mallocProfiler::allThreads);
  std::cout << "===                ===" << std::endl;

  mallocProfiler::setRemangler(nullptr);
  printf("THIS IS THE END\n");
  fflush(stdout);

  return 0;
}
