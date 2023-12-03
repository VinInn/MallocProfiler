#include<vector>
#include<iostream>
#include<cmath>

int main(int argc, char**argv) {
  int k=0;
  {
  std::vector<int> a(100,4);
  auto d = a.data();
  k = d[5];
  }
  std::cout << "till here all all" << std::endl;

  {
    std::vector<int> a(100,4);
    auto d = a.data() -7;
    k = d[5]; // ops
    // make sure we use these
    d[4-argc] = k+argc; // double ops
    k = std::log(d[3]);
  }


  return k;
}
