#include "../Client/slot_pool.hpp"
#include <iostream>
#include <string>

int main() {

  using pool = slot_recycling_pool<std::string, 10>;
  pool string_pool;

  pool::acquire_t string_slot = string_pool.acquire();
  if (string_slot.acquisition_successful)
    std::cout << "Acquisition Successful\n";
  std::cout << "slot generation: " << string_slot.acquired->generation() << '\n';

  auto& str = string_slot.acquired->object;

  str = "yo!";

  std::cout << str << '\n';
  std::cout << "active slots: " << string_pool.active()<< ' ' << "available slots: " << string_pool.available() << '\n';
  string_pool.release(string_slot.acquired);
  std::cout << "active slots: " << string_pool.active()<< ' ' << "available slots: " <<string_pool.available() << '\n';
  std::cout << "slot generation: " << string_slot.acquired->generation() << '\n';

  pool::acquire_t string_slot_2 = string_pool.acquire();
  if (string_slot.acquisition_successful)
    std::cout << "Acquisition Successful\n";

  auto& str_2 = string_slot_2.acquired->object;
  std::cout << str_2 << '\n';
  string_pool.release(string_slot_2.acquired);
  std::cout << "slot_2 generation: " << string_slot_2.acquired->generation() << '\n';
}
