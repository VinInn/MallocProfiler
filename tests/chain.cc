#include<vector>
#include<iostream>

#ifdef ALIB
void b(std::vector<int*>&v);
void a(std::vector<int*>&v) {
   std::cout << "in alib" << std::endl;
   v.push_back(new int(3));
   b(v);
}
#endif

#ifdef BLIB
#include <iostream>
//#include <stacktrace>
void b(std::vector<int*>&v) { 
   std::cout << "in blib" << std::endl;
   v.push_back(new int(4));
//   std::cout << std::stacktrace::current() << std::endl;
}
#endif




