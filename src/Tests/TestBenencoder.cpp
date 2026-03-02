#include "../Bencoder/Bencode.h"
#include "UnitTester.h"

int main() {
  UnitTest ben_encoder_tester{};
  ben_encoder_tester.add_test("Ben-encoder for list", Bendata::encode_to_list, "li755ee", std::vector<std::string>{"i755e"});
  auto overload_wrapper = [](int x){ return Bendata::encode(x); };
  ben_encoder_tester.add_test("Ben-encoder for int", overload_wrapper, "i45e", 45);
  auto overload_wrapper_2 = [](std::string x) { return Bendata::encode(x); };
  ben_encoder_tester.add_test("Ben-encoder for string",overload_wrapper_2, "4:love", std::string("love"));
  BenDictPair a{"peer_id", "20:-AZ2060-xYz123456789"}; BenDictPair b{"port", "i6881e"};
  BenDictPair c{"event", "7:started"}; BenDictPair d{"uploaded", "i349002e"};
  ben_encoder_tester.add_test("Ben-encoder for dict", Bendata::encode_to_dict,
    "d5:event7:started7:peer_id20:-AZ2060-xYz1234567894:porti6881e8:uploadedi349002ee",std::vector<BenDictPair>{a, b, c, d});
  ben_encoder_tester.run_tests();
}
