#include <iostream>

int main(){
  unsigned int a = 1;
  char* b = (char*) &a;
  std::cout << (*b ? "Little endian" : "Big endian") << '\n';
}
