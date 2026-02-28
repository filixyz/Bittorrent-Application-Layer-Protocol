#include <initializer_list>
#include <thread>
#include <type_traits>
#include <deque>
#include <iostream>
#include <functional>
#include "../Bencoder/Bencode.h"
#include <chrono>
#include <string>

// when laptop charged overload add test; one for variadic arguemnst to be forwarded
// the other for initializer_list to be passed to test function, since they behave
// abnormally when used in the context of variadic templates


namespace color
{
  inline std::ostream& reset (std::ostream& os) { return os << "\033[0m"; }
  inline std::ostream& red   (std::ostream& os) { return os << "\033[31m"; }
  inline std::ostream& green (std::ostream& os) { return os << "\033[32m"; }
}

class UnitTest {
  std::deque<std::function<void()>> test_list;
public:
//  template <typename F, typename ER, typename ...A>
//  void add_test(std::string test_tag, F func, ER expected_result,A&&... args) {
//    auto test = [=](){
//      using return_type = std::decay_t< typename std::invoke_result_t<F,A...> >;
//      return_type return_value = func(std::forward<A>(args)...);
//      std::remove_pointer_t<return_type> return_value_nptr;
//      if constexpr (std::is_pointer_v<return_type>)
//        return_value_nptr = *return_value;
//      else
//        return_value_nptr = return_value;
//      bool result_equal = return_value == expected_result;
//      std::cout << return_value << '\n';
//      std::cout  << "Descriptor: " << "\""<< test_tag << "\"" << (result_equal?" succeeded" :" failed");
//      std::cout << "(expected value) " << expected_result << " (returned value) " << return_value << '\n';
//    };
//    test_list.push_back(test);
//  }
//
  template <typename F, typename ER, typename A>
  void add_test(std::string test_tag, F func, ER expected_result, A&& arg) {
    auto test = [=](){
      using return_type = std::decay_t< typename std::invoke_result_t<F, A>>;
      return_type return_value = func(arg);
      std::remove_pointer_t<return_type> return_value_nptr;
      if constexpr (std::is_pointer_v<return_type>)
        return_value_nptr = *return_value;
      else
        return_value_nptr = return_value;
      bool result_equal = return_value_nptr == expected_result;
      std::cout  << "Descriptor: " << (result_equal?color::green:color::red)  << "\""<< test_tag << "\"" << (result_equal?" succeeded" :" failed");
      std::cout <<"\n\t" <<" (expected value) " << expected_result << "\n\t (returned value) " << return_value_nptr << '\n';
      std::cout << color::reset;
    };
    test_list.push_back(test);
  }

  void run_tests() {
    int test_size = test_list.size();
    for (int test_index=0; test_index<test_size; ++test_index) {
      std::cout << "Test [" << test_index+1 << '/' << test_size << "] -> ";
      auto test = test_list.front();
      test_list.pop_front();
      test();
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }
};

int main() {
  UnitTest ben_encoder_tester{};
  ben_encoder_tester.add_test("Ben-encoder for list", Bendata::encode_to_list, "li755ee", std::vector<std::string>{"i755e"});
  auto overload_wrapper = [](int x){ return Bendata::encode(x); };
  ben_encoder_tester.add_test("Ben-encoder for int", overload_wrapper, "i45e", 45);
  auto overload_wrapper_2 = [](std::string x) { return Bendata::encode(x); };
  ben_encoder_tester.add_test("Ben-encoder for string",overload_wrapper_2, "4:love", std::string("lovess"));
  BenDictPair a{"peer_id", "20:-AZ2060-xYz123456789"}; BenDictPair b{"port", "i6881e"};
  BenDictPair c{"event", "7:started"}; BenDictPair d{"uploaded", "i349002e"};
  ben_encoder_tester.add_test("Ben-encoder for dict", Bendata::encode_to_dict,
    "d5:event7:started7:peer_id20:-AZ2060-xYz1234567894:porti6881e8:uploadedi349002ee",std::vector<BenDictPair>{a, b, c, d});
  ben_encoder_tester.run_tests();
}
