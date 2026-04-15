#include <algorithm>
#include <vector>
#include <iostream>

int main() {
  std::vector<int> test;
  test.push_back(1);
  test.push_back(2);
  test.push_back(3);
  test.push_back(4);
  auto print = [](const int& a){ std::cout << a << ' '; };

  std::cout << "Before doing what i want to test \n";
  std::for_each(test.begin(), test.end(), print);
  std::cout << '\n';

  // Test
  auto back_ptr = &test.back();
  *back_ptr = 100;

  std::cout << "After doing what i want to test \n";
  std::for_each(test.begin(), test.end(), print);
  std::cout << '\n';
}
