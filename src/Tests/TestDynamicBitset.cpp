#include "../Client/DynamicBitset.h"
#include <cstdint>
#include <iostream>
int main() {
  std::vector<std::uint8_t> bytes { 0XD5, 0x03 };
  std::vector<std::uint8_t> bytes2 { 0x2A };
  DynamicBitset newset1 (bytes);
  DynamicBitset newset2 (bytes2);
  (newset1 |= newset2);
  newset1.print();
  std::cout << "--------------\n" ;
  (newset1 & newset2).print();
}
