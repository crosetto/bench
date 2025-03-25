#include <chrono>
#include <vector>
#include <iostream>
#include <stdio.h>

int main(){


  std::vector<std::int32_t> v(2<<25, 0); // 128 MiB
  volatile int val = 1;
  int size = v.size();

  for(int stride = 1; stride < 20; ++stride)
    {
      auto start = std::chrono::high_resolution_clock::now();
      for(int i=0; i<size; ++i)
        v[(i*stride)%v.size()] = val;
      auto stop = std::chrono::high_resolution_clock::now();
      double elapsed = (stop - start).count();
      elapsed /= 1.e7;
      // for(int i=0; i< elapsed/1.e7; ++i)
      //   std::cout<<"*";
      // std::cout<<"\n";
      //std::cout<<"s"<<stride<< " = " << elapsed/1.e7<<"\n";
      printf("s%d = %lf\n", stride, elapsed);
    }
}
