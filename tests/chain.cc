#include<vector>
#include<iostream>

#ifdef ALIB
namespace BBB {
void b(std::vector<int*>&v);
}
void a(std::vector<int*>&v) {
   std::cout << "in alib" << std::endl;
   v.push_back(new int(3));
   BBB::b(v);
}
#endif

#ifdef BLIB
#include <iostream>
//#include <stacktrace>
namespace BBB {
void b(std::vector<int*>&v) { 
   std::cout << "in blib" << std::endl;
   v.push_back(new int(4));
//   std::cout << std::stacktrace::current() << std::endl;
}
}
#endif




