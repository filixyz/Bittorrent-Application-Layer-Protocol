#include <array>
#include <string>
#include <iostream>

consteval std::array<char, 20> get_client_id() {
  std::array<char, 20> id;
  const char id_with_null  [21] = "FJ0001-x4Kn8mR2pL9sq";
  for (auto i = 0; i < 20; ++i)
    id[i] = id_with_null[i];
  return id;
}

int main() {
  auto id = get_client_id();
  std::cout << std::string(id.data(), id.size());
}
