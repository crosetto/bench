#include <chrono>
#include <vector>
#include <iostream>

int main(){


  std::vector<std::int32_t> v(2<<25, 0); // 128 MiB
  volatile int val = 1;
  int size = v.size();

  for(int stride = 1; stride < 128; ++stride)
    {
      auto start = std::chrono::high_resolution_clock::now();
      for(int i=0; i<size; ++i)
        v[(i*stride)%v.size()] = val;
      auto stop = std::chrono::high_resolution_clock::now();
      auto elapsed = (stop - start).count();
      // for(int i=0; i< elapsed/1.e7; ++i)
      //   std::cout<<"*";
      // std::cout<<"\n";
      std::cout<<elapsed/1.e7<<"\n";
    }
}
