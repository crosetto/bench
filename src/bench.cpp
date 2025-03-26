#define UNCERTAINTY
#include <chrono>
#include <vector>
#include <array>
#include <iostream>
#ifdef UNCERTAINTY
#include "uxhw.h"
#endif

using namespace std::literals;

auto bandwidth(){
 std::vector<std::int32_t> v(2<<25, 0); // 128 MiB
 volatile int val = 1;
 auto start = std::chrono::high_resolution_clock::now();
 for(auto& i : v){
   i=val;
 }
 auto stop = std::chrono::high_resolution_clock::now();
 double elapsed = (stop - start)/1.s;
 std::cout<<"bandwidth"<< " = " << v.size()*4/elapsed/1e9<<" GB/s\n";
 return elapsed;
 }

auto peak_performance(){
  using float_t = double;
  std::size_t size = 1e6;
  constexpr int simd=8;
  volatile float_t va = 0.f, vb = 0.f;
  alignas(32) std::array<float_t, simd> a;
  alignas(32) std::array<float_t, simd> b;
  alignas(32) std::array<float_t, simd> arr;
  auto start = std::chrono::high_resolution_clock::now();

  for(auto j=0; j<simd; ++j){
    a[j] = va;
    b[j] = vb;
    arr[j] = 0.f;
  }
  
  for(std::size_t i=0; i<size; ++i){
    for(auto j=0; j<simd; ++j){
      auto fma0 = a[j] + b[j]*a[j];
      auto fma1 = a[j] - b[j]*arr[j];
      auto fma2 = arr[j] + a[j]*arr[j];
      auto fma3 = fma0 + (fma1)*(fma2);
      auto fma4 = fma1 + (fma2)*(fma0);
      auto fma5 = fma2 + (fma1)*(fma0);
      auto fma6 = fma3 + (fma4)*(fma5);
      auto fma7 = fma4 + (fma3)*(fma5);
      arr[j] += (fma6)*(fma7); //fma8 : 18 flops per iteration
    }
  }

  double flops = size*simd*18;
  auto stop = std::chrono::high_resolution_clock::now();
  double elapsed = (stop - start)/1.s;
  std::cout<<"performance"<< " = " << flops/elapsed/1e9<<" GFLOP/s\n";
  for(auto j=0; j<simd; ++j){
    if(!arr[j])
      std::cout<<" ";
  }
  return elapsed;
  
}

int main(){


  int samples = 10;
  std::vector<double> band(samples);
  std::vector<double> perf(samples);
  
  for(int i=0; i<samples; ++i){
    band[i] = bandwidth();
    perf[i] = peak_performance();
  }

#ifdef UNCERTAINTY
  auto b = UxHwDoubleDistFromSamples(band.data(), band.size());
  auto p = UxHwDoubleDistFromSamples(perf.data(), perf.size());
  auto arithmetic_intensity = UxHwDoubleUniformDist(0., 5.0);
  std::cout<<"b = "<<b<<"\n";
  std::cout<<"p = "<<p<<"\n";
  std::cout<<"arithmetic_intensity = "<<arithmetic_intensity<<"\n";
  
  auto roofline = [&](double ai){
    return std::max(b*ai, p);
  };

  std::cout<<"r = "<<roofline(arithmetic_intensity)<<"\n";
#endif
}
