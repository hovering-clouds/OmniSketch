/**
 * @file ACS_CMSketch.h
 * @author dromniscience (you@domain.com)
 * @brief Implementation of Count Min Sketch
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include "common/hash.h"
#include <common/sketch.h>
#include <common/ACScounter.h>
using OmniSketch::Counter::ACScounter;

namespace OmniSketch::Sketch {
/**
 * @brief Count Min Sketch
 *
 * @tparam key_len  length of flowkey
 * @tparam T        type of the counter
 * @tparam hash_t   hashing class
 */
template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACS_CMSketch : public SketchBase<key_len, T> {
private:
  int32_t depth;
  int32_t width;
  const int32_t offset;
  hash_t *hash_fns;
  ACScounter<T>& counter;

  ACS_CMSketch(const ACS_CMSketch &) = delete;
  ACS_CMSketch(ACS_CMSketch &&) = delete;

public:
  /**
   * @brief Construct by specifying depth and width
   * @param width_ should be prime number to reduce hash collision
   *
   */
  ACS_CMSketch(int32_t depth_, int32_t width_, int32_t _offset, ACScounter<T>& counter_);
  /**
   * @brief Release the pointer
   *
   */
  ~ACS_CMSketch();
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
  /**
   * @brief Get the size of the sketch
   *
   */
  size_t size() const override;
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, typename T, typename hash_t>
ACS_CMSketch<key_len, T, hash_t>::ACS_CMSketch(int32_t depth_, int32_t width_, int32_t _offset, ACScounter<T> &counter_)
    : depth(depth_), width(width_), counter(counter_), offset(_offset){
  hash_fns = new hash_t[depth];
}

template <int32_t key_len, typename T, typename hash_t>
ACS_CMSketch<key_len, T, hash_t>::~ACS_CMSketch()
{
    delete[] hash_fns;
}

template <int32_t key_len, typename T, typename hash_t>
void ACS_CMSketch<key_len, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                          T val) {
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    counter.update(index, val);
  }
}

template <int32_t key_len, typename T, typename hash_t>
T ACS_CMSketch<key_len, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    min_val = std::min(min_val, counter.query(index));
  }
  return min_val;
}

template <int32_t key_len, typename T, typename hash_t>
size_t ACS_CMSketch<key_len, T, hash_t>::size() const {
  return sizeof(*this)                // instance
         + sizeof(hash_t) * depth     // hashing class
         + sizeof(T) * depth * width; // counter
}

} // namespace OmniSketch::Sketch
