#include <iostream>
#include <bitset>

int main() {
  short i;
  int a = 0xFFFFFFFF;
  i = a;
  std::cout << std::hex << std::bitset<16>(i).to_ulong() << '\n';
}
