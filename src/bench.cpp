//#define UNCERTAINTY
#include <chrono>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#ifdef UNCERTAINTY
#include "uxhw.h"
#endif

// toy code to estimate a roofline performance model for 1 core of a microprocessor:
// - estimates the hardware bandwidth by writing data to a std::vector
// - estimates the hardware peak FLOP/s by running many vectorized FMA instructions on cached data
// for an application, the arithmetic intensity is the ratio between FLOPs executed and bytes accessed
// we assume the arithmetic intensity for typical user codes is uniformly distributed in (0.1, 5) 

using namespace std::literals;

// estimates the (write) memory bandwidth by copying 
auto bandwidth(){
 std::vector<std::int32_t> v(2<<25, 0); // 128 MiB
 volatile int val = 1; // volatile, so that the compiler does not optimize away the loop

 // measuring how much it takes to copy the vector
 auto start = std::chrono::high_resolution_clock::now();
 for(auto& i : v){
   i=val;
 }
 auto stop = std::chrono::high_resolution_clock::now();
 double elapsed = (stop - start)/1.s;
 std::cout<<"bandwidth"<< " = " << v.size()*4/elapsed/1e9<<" GB/s\n";
 return elapsed;
 }

// when compiled with the proper flags (e.g. -O3 -march=native in GCC/Clang), the code auto-vectorizes
// generates 9 fused-multiply-add instructions per iteration
auto peak_performance(){
  using float_t = double;
  std::size_t size = 1e6;
  constexpr int simd=8;
  volatile float_t va = 0.f, vb = 0.f; // volatile, so the compiler does not make assumptions
  alignas(32) std::array<float_t, simd> a; // these arrays typically fit in L1 cache
  alignas(32) std::array<float_t, simd> b;
  alignas(32) std::array<float_t, simd> arr;

  for(auto j=0; j<simd; ++j){
    arr[j] = 0.f;
    b[j] = vb;
  }

  auto start = std::chrono::high_resolution_clock::now();
  for(std::size_t i=0; i<size; ++i){
    for(auto j=0; j<simd; ++j){
      a[j] = va; // inhibits compiler optimizations
    }
  
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
  auto stop = std::chrono::high_resolution_clock::now();

  double flops = size*simd*18; // total number of flops
  double elapsed = (stop - start)/1.s;
  std::cout<<"performance"<< " = " << flops/elapsed/1e9<<" GFLOP/s\n";

  if(!std::ranges::max(arr)) // makes sure the loop is not optimized away by the compiler
    std::cout<<" ";
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
  // creates a distribution based on the measured times
  auto b = UxHwDoubleDistFromSamples(band.data(), band.size());
  auto p = UxHwDoubleDistFromSamples(perf.data(), perf.size());

  // creates a distribution for the arithmetic intensity of the "user codes"
  auto arithmetic_intensity = UxHwDoubleUniformDist(0.1, 5.0);

  std::cout<<"b = "<<b<<"\n";
  std::cout<<"p = "<<p<<"\n";
  std::cout<<"arithmetic_intensity = "<<arithmetic_intensity<<"\n";
  
  auto roofline = [&](double const& ai){
    return std::max(b*ai, p);
  };

  //the highes the average of this number, the better (performance-wise) our hardware adapts to the user codes
  std::cout<<"r = "<<roofline(arithmetic_intensity)<<"\n";
#endif
}
