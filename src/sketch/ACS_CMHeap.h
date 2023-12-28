/**
 * @file ACS_CMHeap.h
 * @author hc (you@domain.com)
 * @brief Implementation of Count Min Sketch with ACS counters
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "common/hash.h"
#include <common/sketch.h>
#include <common/ACScounter.h>
#include <queue>
#include <map>

namespace OmniSketch::Sketch {
/**
 * @brief Count Min Sketch
 *
 * @tparam key_len  length of flowkey
 * @tparam T        type of the counter
 * @tparam hash_t   hashing class
 */
template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACS_CMHeap : public SketchBase<key_len, T> {
private:
  class Item{
  private:
    FlowKey key;
    T val;
  public:
    Item(FlowKey& key_, T val_): key(key_), val(val_) {};
    ~Item(){};
    bool operator<(const Item& other) const {
      return this->val<other.val;
    }
    bool operator>(const Item& other) const {
      return this->val>other.val;
    }
  };

  int32_t depth;
  int32_t width;
  const int32_t offset;
  hash_t *hash_fns;
  Counter::ACScounter<T>& counter;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;

  ACS_CMHeap(const ACS_CMHeap &) = delete;
  ACS_CMHeap(ACS_CMHeap &&) = delete;

public:
  /**
   * @brief Construct by specifying depth and width
   * @param width_ should be prime number to reduce hash collision
   *
   */
  ACS_CMHeap(int32_t depth_, int32_t width_, int32_t _offset, Counter::ACScounter<T>& counter_);
  /**
   * @brief Release the pointer
   *
   */
  ~ACS_CMHeap();
  /**
   * @brief Update a flowkey with certain value
   *
   */
  void update(const FlowKey<key_len> &flowkey, T val) override;
  /**
   * @brief Query a flowkey
   *
   */
  T query(const FlowKey<key_len> &flowkey) const override;

  T est(const FlowKey<key_len> &flowkey) const override;
  /**
   * @brief Get the size of the sketch
   *
   */
  size_t size() const override;
  size_t cntNum() const override;
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, typename T, typename hash_t>
ACS_CMHeap<key_len, T, hash_t>::ACS_CMHeap(int32_t depth_, int32_t width_, int32_t _offset, Counter::ACScounter<T> &counter_)
    : depth(depth_), width(width_), counter(counter_), offset(_offset){
  hash_fns = new hash_t[depth];
}

template <int32_t key_len, typename T, typename hash_t>
ACS_CMHeap<key_len, T, hash_t>::~ACS_CMHeap()
{
    delete[] hash_fns;
}

template <int32_t key_len, typename T, typename hash_t>
void ACS_CMHeap<key_len, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                          T val) {
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    counter.update(index, val);
  }
  
}

template <int32_t key_len, typename T, typename hash_t>
T ACS_CMHeap<key_len, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    min_val = std::min(min_val, counter.query(index));
  }
  return min_val;
}

template <int32_t key_len, typename T, typename hash_t>
T ACS_CMHeap<key_len, T, hash_t>::est(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    min_val = std::min(min_val, counter.est(index));
  }
  return min_val;
}

template <int32_t key_len, typename T, typename hash_t>
size_t ACS_CMHeap<key_len, T, hash_t>::size() const {
  return sizeof(*this)                // instance
         + sizeof(hash_t) * depth     // hashing class
         + sizeof(T) * depth * width; // counter
}

template <int32_t key_len, typename T, typename hash_t>
size_t ACS_CMHeap<key_len, T, hash_t>::cntNum() const {
  return depth * width;
}

} // namespace OmniSketch::Sketch
