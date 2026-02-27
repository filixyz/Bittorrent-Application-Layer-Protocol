#include "../Bencoder/Bencode.h"
#include <cstddef>
#include <type_traits>
#include <cstring>

struct Tester {
  int test_count;
  int test_counter;
  Tester(int count) : test_count(count), test_counter(count) {}

  template <typename F, typename ER, typename ...A>
  void test(F func, ER expected_result,A...args ) {
    if (test_counter!=0) { --test_counter; return; }
    using return_type = std::result_of<F(A...)>::type;
    return_type return_value = func(args...);
    std::size_t size = sizeof(std::decay_t<ER>);
    if (std::is_pointer<return_type>::value && std::is_pointer<ER>::value) {
      bool equal = std::memcmp(return_value, expected_result, size);
      std::cout << "Test " << test_count-test_counter << (equal?" succeeded":" failed") << '\n';
      return;
    } else {
      bool equal = std::memcmp(&return_value, &expected_result, size);
      std::cout << "Test " << test_count-test_counter << (equal?" succeeded":" failed") << '\n';
      return;
    }
  }
  //template<typename T, typename MF, typename R>
  //void test(T type , MF member_function, R expected_result ) {
  //}
};

int main() {
  Tester ben_encoder_tester(1);
  ben_encoder_tester.test(Bendata::encode, 788, std::string("i788e"));
}
