#include "../include/argumentRemover.h"

#include <typeinfo>
#include <iostream>
#include <tuple>
#include<vector>
#include<string>

template<typename T> 
struct W {
  T v;
  W(T t) : v(t){}

};

using WWI = W<W<W<W<int>>>>;
using WWF = W<W<W<W<float>>>>; 
using P = std::pair<WWI,WWF>;
using V = std::vector<P>;
using VV = std::vector<V>;
using VVV = std::vector<std::pair<std::string,VV>>;

void testRemover(std::string symNameV) {

  std::cout << "Original " << symNameV << std::endl;

  removeTemplate(symNameV,"std::allocator");
  std::cout << "Cleaned " << symNameV << std::endl;
  if (std::string::npos!=symNameV.find("std::allocator")) std::cout << "FAILED" << std::endl;
}

int main() {

  std::cout << typeid(V).name() << std::endl;

  std::string symNameV = "std::vector<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > >, std::allocator<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > > > >";


  testRemover(symNameV);

  std::cout << typeid(VVV).name() << std::endl;

  std::string symNameVVV = "std::vector<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::vector<std::vector<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > >, std::allocator<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > > > >, std::allocator<std::vector<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > >, std::allocator<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > > > > > > >, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::vector<std::vector<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > >, std::allocator<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > > > >, std::allocator<std::vector<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > >, std::allocator<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > > > > > > > > >";


  testRemover(symNameVVV);


  return 0;
}
