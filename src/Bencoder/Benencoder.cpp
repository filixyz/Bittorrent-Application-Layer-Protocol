#include "Bencode.h"
#include <string>
#include <algorithm>

std::string Bendata::encode(int value){
  return std::string{'i'} + std::to_string(value) + 'e';
}
std::string Bendata::encode(const std::string& value){
  return std::string{} + std::to_string(value.length()) + ':' + value;
}
std::string Bendata::encode_to_list(std::initializer_list<std::string> items) {
  std::string res{'l'};
  for (auto& i : items) res+=i;
  return res += 'e';
}
std::string Bendata::encode_to_dict(std::initializer_list<BenDictPair> items) {
  std::vector items_vec(items);
  std::sort(items_vec.begin(), items_vec.end(),
    [](BenDictPair& a, BenDictPair& b){ return a.key < b.key; });
  std::string encoded_result("d");
  for (auto& pair : items_vec) encoded_result += encode(pair.key) + pair.bencoded_value;
  encoded_result += "e";
  return encoded_result;
}
