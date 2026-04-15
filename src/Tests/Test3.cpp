#include <vector>
#include <iostream>

int main() {
  std::vector<int> list {1, 2, 3 ,5};
  if(*list.end() == list[list.size()]) {
    std::cout << "YES\n";
  }
}
