#include "../include/mallocProfiler.h"

#include<vector>
#include<atomic>

std::atomic<double> dummy = 0;


#include<cstdio>
#include<sstream>
#include<iostream>
#include<iomanip>


#include <mutex>
#include <thread>
#include <condition_variable>

  typedef std::thread Thread;
  typedef std::vector<std::thread> ThreadGroup;
  typedef std::mutex Mutex;
  typedef std::unique_lock<std::mutex> Lock;
  typedef std::condition_variable Condition;

  std::atomic<int> nt = 0;
  Mutex outLock;
 

template<typename T, int mode>
void go(int size) {

  std::cout << "thread wait" << std::endl;
  nt++;
  while (0!=nt) std::this_thread::yield();
  std::cout << "thread go" << std::endl;

  std::vector<int> v;
  if (mode==1) v.reserve(size);

  for (int i=0; i<size; ++i)
    v.push_back(T(i));

  dummy +=v[5];

  std::ostringstream out;
  // the buffer will be allocated but not tracked, the frwill ... 
  mallocProfiler::dump(out, ' ', mallocProfiler::SortBy::max, mallocProfiler::currentThread);
  {Lock(outlock); std::cout << out.str() << std::endl;}

}




int main() {

  int NTHREADS=8;

  printf("START  MT\n");
  fflush(stdout);


  std::cout << "profiler status " << std::boolalpha << mallocProfiler::active(mallocProfiler::allThreads) << ' ' << mallocProfiler::active(mallocProfiler::currentThread) << std::endl;

  {
    std::cout << "first thread alloc  second thread delete (once)" << std::endl;
    int * vi;
    Thread a([&](){ vi = new int[1000];});
    a.join();
    Thread b([&](){ delete [] vi;});
    b.join();
    mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max, mallocProfiler::allThreads); 
  }



  std::cout << std::endl;


  std::cout << NTHREADS << "self contained" << std::endl; 
  ThreadGroup threads;
  threads.reserve(NTHREADS);
  nt=0;
  for (int i=0; i<NTHREADS; ++i) {
    threads.emplace_back(go<int,0>,1000);
  }
  while (nt<NTHREADS)  std::this_thread::yield();
  std::cout << "threads ready" << std::endl;
  nt=0;
  for ( auto & t :  threads) t.join();


  std::vector<int *> vvv(2049,nullptr);
  std::cout <<  "stress alloc"  << std::endl;
  nt =0;
  for (int i=0; i<2049; ++i) {
    Thread t([&](int k){vvv[k]=new int[1000]; nt++;},i);
    t.detach();
  }
  while (nt<2049)  std::this_thread::yield();
  std::cout <<  "stress delete "  << std::endl;
  nt=0;
  for (int i=0; i<2049; ++i) {
    Thread t([&](int k){delete vvv[k]; nt++;},i);
    t.detach();
  }
  while (nt<2049)  std::this_thread::yield();

  std::cout << "=== Global Profile ===" << std::endl;
  mallocProfiler::dump(std::cout, ' ', mallocProfiler::SortBy::max,  mallocProfiler::allThreads);
  std::cout << "===                ===" << std::endl;


  printf("THIS IS THE END\n");
  fflush(stdout);


  return 0;
}
