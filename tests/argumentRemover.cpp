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


int main() {

  std::cout << typeid(V).name() << std::endl;

  std::string symName = "std::vector<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > >, std::allocator<std::pair<W<W<W<W<int> > > >, W<W<W<W<float> > > > > > >";

  std::cout << symName << std::endl;

  auto pos = symName.find(", std::allocator");
  auto p = argumentRemover(symName,pos);
  if (p!=std::string::npos)  symName.erase(pos, p-pos+1);
  std::cout << symName << std::endl;

  return 0;
}
