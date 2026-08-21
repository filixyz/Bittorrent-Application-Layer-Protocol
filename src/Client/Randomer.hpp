#pragma once

#include <cstdlib>
#include <random>
#include <concepts>

template <typename T>
class randomer {
protected:
  std::mt19937 gen;
public:
  randomer(): gen(std::random_device()()) {}
  virtual T get() = 0;
  virtual ~randomer() = default;
};

template <std::integral int_t>
class int_randomer: public randomer<int_t> {
  std::uniform_int_distribution<int_t> int_dist;
public:
  int_randomer (int_t begin, int_t end): randomer<int_t>(), int_dist(begin, end) {}
  int_t get() override { return int_dist(this->gen); }
};

template <std::floating_point float_t>
class float_randomer: public randomer<float_t> {
  std::uniform_real_distribution<float> flt_dist;
public:
  float_randomer (float_t begin, float_t end): randomer<float_t>(), flt_dist(begin, end) {}
  float_t get() override { return flt_dist(this->gen); }
};
