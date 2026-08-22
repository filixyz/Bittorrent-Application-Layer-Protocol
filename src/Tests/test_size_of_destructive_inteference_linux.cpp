#include <iostream>
#include <new>

int main() {
  std::cout << std::hardware_destructive_interference_size<< " : destrcutive"<< '\n';
  std::cout << std::hardware_constructive_interference_size<< " : constrcutive"<< '\n';
}
