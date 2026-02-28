#include <initializer_list>
#include <type_traits>
#include <deque>
#include <iostream>
#include <functional>
#include <utility>
#include "../Bencoder/Bencode.h"

// when laptop charged overload add test; one for variadic arguemnst to be forwarded
// the other for initializer_list to be passed to test function, since they behave
// abnormally when used in the context of variadic templates
struct UnitTest {
  std::deque<std::function<void()>> test_list;

  template <typename F, typename ER, typename ...A>
  void add_test(std::string test_tag, F func, ER expected_result,A&&... args) {
    auto test = [=](){;
      using return_type = std::decay_t< typename std::invoke_result_t<F,A...> >;
      return_type return_value = func(std::forward<A>(args)...);
      std::remove_pointer_t<return_type> return_value_nptr;
      if constexpr (std::is_pointer_v<return_type>)
        return_value_nptr = *return_value;
      else
        return_value_nptr = return_value;
      bool result_equal = return_value == expected_result;
      std::cout << return_value << '\n';
      std::cout  << "Descriptor " << test_tag << (result_equal?" succeeded" :" failed") << '\n';
    };
    test_list.push_back(test);
  }
  template <typename F, typename ER, typename A>
  void add_test(std::string test_tag, F func, ER expected_result, A arg) {
    auto test = [=](){
      using return_type = std::decay_t< typename std::invoke_result_t<F, A>>;
      return_type return_value = func(arg);
      std::remove_pointer_t<return_type> return_value_nptr;
      if constexpr (std::is_pointer_v<return_type>)
        return_value_nptr = *return_value;
      else
        return_value_nptr = return_value;
      bool result_equal = return_value == expected_result;
      std::cout << return_value << '\n';
      std::cout  << "Descriptor " << test_tag << (result_equal?" succeeded" :" failed") << '\n';
    };
    test_list.push_back(test);
  }
  void run_tests() {
    int test_size = test_list.size();
    for (int test_index=0; test_index<test_size; ++test_index) {
      std::cout << "test " << test_index+1 << '/' << test_size << " -> ";
      auto test = std::move(test_list.front());
      test();
      test_list.pop_front();
    }
  }
};

int main() {
  UnitTest ben_encoder_tester{};
  ben_encoder_tester.add_test("Ben-encoder for list", Bendata::encode_to_list, "li755ee", std::initializer_list<std::string>{"i755e"});
  ben_encoder_tester.run_tests();
}
