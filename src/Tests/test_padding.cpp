#include <cstdint>
#include <iostream>

struct test {   // Addreess line; value mulitiple of let A = 8
  uint64_t  a;  //  8 bytes
  int       b;  //  4 bytes
  bool      c;  //  1 byte
  bool      d;  //  1 byte
  // total      -> 14 bytes
  // exepected address = A + 14 = 22
  // 22 cannot work since not divisible by 8, 2 extra bytes needed
  // paddding 2 bytes extra;
  //
  // Hence size of this structure should be 16 bytes;
  //
  // --------------we could add our own padding tho---------------
  //
//  uint8_t padding1;
//  uint8_t padding2;
  // still 16 bytes but structure now predictable;
};

int main() {
  std::cout << sizeof(test) << " bytes = test_t size\n";
}
