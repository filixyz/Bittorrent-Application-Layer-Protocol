#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <bitset> // for test
#include <chrono>

class my_bitset {
  std::vector<unsigned long long> bitss;
public:
  unsigned long long bit_count;

  my_bitset(unsigned long long bits): bit_count(bits)  {
    unsigned long long   extra = bits%64;
    unsigned long long   extra_bits = (bool)extra ? extra : 64;
    unsigned long long   size = (bool)extra ? (unsigned long long)(bits/64)+1 : (unsigned long long)(bits/64);
    bitss = std::vector<unsigned long long>(size);
    bitss.shrink_to_fit();

    for (int start = 0; start != bitss.size(); ++start) {
      //if (start == bitss.size()-1) {
      //  bitss[start] = (unsigned long long) std::exp2(extra_bits)-1;
      //  continue;
      //}
      //bitss[start] = 0xFFFFFFFFFFFFFFFF;
      bitss[start] = 0;
    }

  }

  void print() {
    for (int start = bitss.size()-1; start != -1; --start) {
      std::cout << std::hex << bitss[start] << '\n';
    } 
  }
};

int main() {
  auto start = std::chrono::steady_clock::now();
  my_bitset set{100'000'0};
  auto end = std::chrono::steady_clock::now();
  std::cout << "for my_bitset " << set.bit_count << " bits, construction took: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microseconds\n";
  
  std::cout << '\n';

  auto startt = std::chrono::steady_clock::now();
  std::bitset<67'000'000> set2{};
  auto endd = std::chrono::steady_clock::now();
  std::cout << "for std::bitset " << set2.size() << " bits, construction took: " << std::chrono::duration_cast<std::chrono::microseconds>(endd - startt).count() << " microseconds\n";
  
  std::cout << '\n';

  auto starttt = std::chrono::steady_clock::now();
  std::vector<bool> set3 (100'000'0);
  auto enddd = std::chrono::steady_clock::now();
  std::cout << "for std::vector<bool> " << set3.size() << " bits, construction took: " << std::chrono::duration_cast<std::chrono::microseconds>(enddd - starttt).count() << " microseconds\n";

}
