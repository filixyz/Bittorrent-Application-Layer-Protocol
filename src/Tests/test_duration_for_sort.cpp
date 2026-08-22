#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <bitset>

enum from { them, me };

// This code test the worist case scenario of mutex acquisition of the connected peers list
// between tcpTransferManager and PeerManager when (indirectly, or directly) acquiring the mutex
// worst case should be: 
//  1) connected peers vector is full N = 50, so sorting is at max
//  2) all peers are active (knows im interested and are not choking me)

int count = 0;

struct peer;

struct observer {
  peer* a = nullptr;
  observer() = default;
};

struct peer {
  int speed = 0;
  std::bitset<10> bitfield{0x0};
  std::bitset<2> choke{0x0};  // them, me
  std::bitset<2> interest{0x3};
  bool is_piece_set(std::size_t index) {
    return bitfield[index];
  }
};

void rankify_peers(std::vector<observer>& peers) {
  for (int i=0; i < peers.size(); ++i) {
    if (i < 3)
      peers[i].a->choke.set(from::me);
    else
      peers[i].a->choke.reset(from::me);
  }
  // place optimistic unchoke here
}

bool is_active(observer& a) {
  return ( !a.a->choke.test(from::them) ) && a.a->interest.test(from::me);
}

std::vector<observer> get_has_piece(std::vector<observer> connected, int piece) {
  std::vector<observer> has_piece;
  for (auto& i : connected) {
    if(is_active(i) && i.a->bitfield.test(piece)) 
      has_piece.push_back(i);
  }
  return has_piece;
}

int main() {
  std::unordered_map<std::string, peer> storage;
  for (int i = 0; i < 50; ++i)  {
    peer& p = storage[std::to_string(i)] = peer();
    p.speed = std::rand();
    if (count!=0 && count%10==0) std::cout << '\n'; std::cout << ( (i<10) ? std::string("0").append(std::to_string(i)) : std::to_string(i)) << ": "<< p.speed << '\t'; ++count;
  };
  std::cout << '\n';
;
  auto print = [](std::pair<std::string, peer> a) { if (count!=0 && count%10 == 0) std::cout << '\n'; std::cout << a.second.speed << ' '; ++count;};
  std::for_each(storage.begin(), storage.end(), print);
  std::cout << '\n';

  std::vector<observer> observers{};
  for (int i = 0; i < 50; ++i)  {
    observer a = observer();
    a.a = &storage[std::to_string(i)];
    observers.push_back(std::move(a));
  }

  storage["1"].bitfield.set(1);
  storage["6"].bitfield.set(0);
  storage["17"].bitfield.set(1);
  storage["49"].bitfield.set(0);

  auto compare = [](observer a, observer b){ return a.a->speed > b.a->speed; };
  auto begin = std::chrono::steady_clock::now();
  std::sort(observers.begin(), observers.end(), compare);
  rankify_peers(observers);
  (void) get_has_piece(observers, 1);
  auto end   = std::chrono::steady_clock::now();
  std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(end-begin);
  auto print2 = [](observer a) {if(count!=0 && count%10 == 0) std::cout << '\n'; std::cout << a.a->speed << ' '; ++count;};
  std::for_each(observers.begin(), observers.end(), print2); std::cout << '\n';
  auto has_pii = get_has_piece(observers, 1);
  std::for_each(has_pii.begin(), has_pii.end(), print2); std::cout << '\n';
  std::cout << "Sort of observers vector size " << observers.size() << " objects, took " << duration.count() << " microseconds\n";
}
