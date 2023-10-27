#include "../include/mallocProfiler.h"

#include<vector>


double dummy = 0;


#include<cstdio>
#include<iostream>
#include<iomanip>

template<typename T, int mode>
void go(int size) {

  std::vector<int> v;
  if (mode==1) v.reserve(size);

  for (int i=0; i<size; ++i)
    v.push_back(T(i));

  dummy +=v[5];

  
  mallocProfiler::dump(std::cout, mallocProfiler::SortBy::max, false);

}


void recursive(std::vector<int*> & v, int n) {
  if (0==n) return;
  v.push_back(new int(4));
  recursive(v,n-1);
}

#include<iostream>

int main() {

  printf("START\n");
  fflush(stdout);


  std::cout << "profiler status " << std::boolalpha << mallocProfiler::active(true) << ' ' << mallocProfiler::active(false) << std::endl;

  go<int,0>(100);
  go<int,1>(1000);
  go<double,0>(1000);

  std::cout << std::endl;

  std::vector<int*> v;
  recursive(v,257);

  std::cout << v.size() << std::endl;

  return 0;
}
