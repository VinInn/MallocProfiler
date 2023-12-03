// c++ -O3 -pthread -fPIC -shared -o mallocGuard.so plugins/mallocGuard.cc -ldl -DVERIFY_GUARD
#include <cstdint>
#include <cstdlib>
#include <dlfcn.h>
#include <unistd.h>
#include <cstdio>
#include<cassert>
#include<cstring>
#include<algorithm>
#include<atomic>
#include<iostream>

namespace {

#ifndef OFFSET
#define OFFSET 64
#endif

  std::atomic<uint64_t> c=0;

}

extern "C"
{

  typedef void * (*mallocSym) (std::size_t);
  typedef void (*freeSym) (void*);
  typedef void * (*callocSym)(std::size_t, std::size_t); // also for aligned_alloc
  typedef void * (*reallocSym)(void *, std::size_t);

  mallocSym origV = nullptr;
  freeSym origF = nullptr;
  callocSym origA = nullptr;
  callocSym origC = nullptr;
  reallocSym origR = nullptr;


  void *aligned_alloc(std::size_t alignment, std::size_t size ) {
    c++;
    alignment = 64;
    if (!origA) origA = (callocSym)dlsym(RTLD_NEXT,"aligned_alloc");
    assert(origA);
    // add leading and trailing guards (zeroed)
    uint8_t * p =  (uint8_t *)origA(alignment, size+2*OFFSET);
    memset(p,0,OFFSET);
    memset(p+OFFSET+size,0,OFFSET);
    return p+OFFSET;
  }


  void *malloc(std::size_t size) {
    return  aligned_alloc(64,size);
  }

  void * valloc(size_t size) {
   if (!origV) origV = (mallocSym)dlsym(RTLD_NEXT,"valloc");
    assert(origV);
    // add leading and trailing guards zeroed)
    uint8_t * p =  (uint8_t *)origV(size+2*OFFSET);
    memset(p,0,OFFSET);
    memset(p+OFFSET+size,0,OFFSET);
    return p+OFFSET;
  }

  void *calloc(std::size_t count, std::size_t size ) {
    if (!origC) origC = (callocSym)dlsym(RTLD_NEXT,"calloc");
    assert(origC);
    uint8_t * p =  (uint8_t *)origC(count, size+2*OFFSET); // a bit too much....
    return p+OFFSET;
  }

  void *realloc(void *ptr, std::size_t size) {
    if (!origR) origR = (reallocSym)dlsym(RTLD_NEXT,"realloc");
    assert(origR);
    uint8_t * p =  (uint8_t *)ptr;
    if (p) p-=OFFSET;
    // should we verify that are stiil zero?
    p =  (uint8_t *)origR(p,size+2*OFFSET);
    memset(p,0,OFFSET);
    return p+OFFSET;
  }

  void free(void *ptr) {
    if(!origF) origF = (freeSym)dlsym(RTLD_NEXT,"free");
    assert(origF);
    uint8_t * p =  (uint8_t *)ptr;
    if (p) {
      p-=OFFSET;
#ifdef VERIFY_GUARD
      for (int i=0; i<OFFSET; ++i) assert(0==p[i]);
#endif
    }
    origF(p);
  }

}





namespace {

  struct Banner {
    Banner() {
     printf("Malloc wrapper loading in %d\n",getpid());
     fflush(stdout);
   }
   ~Banner() {
    std::cout << "number of mallac calls " << c << std::endl;
  }

  };

 Banner banner;
}

