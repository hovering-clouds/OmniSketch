/**
 * @file ACS_Deltoid.h
 * @author deadlycat <lsmfttb@gmail.com>
 * XierLabber<yangshibo@stu.pku.edu.cn>(modified)
 * hc
 * @brief Implementation of Deltoid with ACS
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <common/hash.h>
#include <common/sketch.h>
#include <common/ACScounter.h>
#include <iostream>
#include <vector>

namespace OmniSketch::Sketch {
/**
 * @brief ACS_Deltoid
 *
 * @tparam key_len  length of flowkey
 * @tparam T        type of the counter
 * @tparam hash_t   hashing class
 */
template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACS_Deltoid : public SketchBase<key_len, T> {
private:
  T sum_;
  int32_t num_hash_;
  int32_t num_group_;
  int32_t nbits_;
  const int32_t offset;
  Counter::ACScounter<T>& counter;
  T* sum_counter;
  hash_t *hash_fns_; // hash funcs

public:
  /**
   * @brief Construct by specifying hash number and group number
   *
   */
  ACS_Deltoid(int32_t num_hash, int32_t num_group, int32_t offset_, Counter::ACScounter<T>& counter_);
  /**
   * @brief Release the pointer
   *
   */
  ~ACS_Deltoid();
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
   * @brief Get all the heavy hitters
   *
   */
  Data::Estimation<key_len, T> getHeavyHitter(
      double threshold) const override; /**
                                         * @brief Get the size of the sketch
                                         *
                                         */
  size_t size() const override;
  /**
   * @brief Reset the sketch
   *
   */
  void clear();
  size_t cntNum() const override;
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of template methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, typename T, typename hash_t>
ACS_Deltoid<key_len, T, hash_t>::ACS_Deltoid(int32_t num_hash, int32_t num_group, int32_t offset_, Counter::ACScounter<T>& counter_)
    : num_hash_(num_hash), num_group_(Util::NextPrime(num_group)),
      nbits_(key_len * 8), sum_(0), offset(offset_), counter(counter_) {
  hash_fns_ = new hash_t[num_hash_];
  sum_counter = new T[num_hash_*num_group_];
  std::fill_n(sum_counter, num_hash_*num_group_, 0);
}

template <int32_t key_len, typename T, typename hash_t>
ACS_Deltoid<key_len, T, hash_t>::~ACS_Deltoid() {
  delete[] hash_fns_;
  delete[] sum_counter;
}

template <int32_t key_len, typename T, typename hash_t>
void ACS_Deltoid<key_len, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                         T val) {
  sum_ += val;
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % num_group_;
    int32_t uidx = offset+(i*num_group_+idx)*nbits_;
    for (int32_t j = 0; j < nbits_; ++j) {
      if (flowkey.getBit(j)) {
        counter.update(uidx+j, val);
      }
    }
    sum_counter[i*num_group_+idx]+=val;
  }
}

template <int32_t key_len, typename T, typename hash_t>
T ACS_Deltoid<key_len, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {

  static bool cnt_distrib = true;

  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % num_group_;
    int32_t uidx = offset+(i*num_group_+idx)*nbits_;
    for (int32_t j = 0; j < nbits_; ++j) {
      if (flowkey.getBit(j)) {
        min_val = std::min(min_val, counter.query(uidx+j));
      } else {
        min_val = std::min(min_val, sum_counter[i*num_group_+idx]-counter.query(uidx+j));
        if(min_val<0){printf("%d %d\n", sum_counter[i*num_group_+idx], counter.query(uidx+j));}
      }
    }
  }
  return min_val;
}

template <int32_t key_len, typename T, typename hash_t>
Data::Estimation<key_len, T>
ACS_Deltoid<key_len, T, hash_t>::getHeavyHitter(double threshold) const {
  T thresh = threshold;
  double val1 = 0;
  double val0 = 0;
  Data::Estimation<key_len, T> heavy_hitters;
  for (int32_t i = 0; i < num_hash_; i++) {
    for (int32_t j = 0; j < num_group_; j++) {
      int32_t uidx = offset+(i*num_group_+j)*nbits_;
      T cntall = sum_counter[i*num_group_+j];
      if (cntall <= thresh) { // no heavy hitter in this group
        continue;
      }
      FlowKey<key_len> fk{}; // create a flowkey with full 0
      bool reject = false;
      for (int32_t k = 0; k < nbits_; k++) {
        T cnt1 = counter.query(uidx+k);
        T cnt0 = cntall-cnt1;
        bool t1 = (cnt1 > thresh);
        bool t0 = (cnt0 > thresh);
        if (t1 == t0) {
          reject = true;
          break;
        }
        if (t1) {
          fk.setBit(k, true);
        }
      }
      if (reject) {
        continue;
      } else if (!heavy_hitters.count(fk)) {
        T esti_val = query(fk);
        heavy_hitters[fk] = esti_val;
      }
    }
  }
  return heavy_hitters;
}

template <int32_t key_len, typename T, typename hash_t>
size_t ACS_Deltoid<key_len, T, hash_t>::size() const {
  return sizeof(ACS_Deltoid<key_len, T, hash_t>) +
         (num_group_ * num_hash_ * nbits_) * sizeof(T) +
         num_hash_ * sizeof(hash_t);
}

template <int32_t key_len, typename T, typename hash_t>
void ACS_Deltoid<key_len, T, hash_t>::clear() {
  sum_ = 0;
  //std::fill(arr0_[0][0], arr0_[0][0] + num_hash_ * num_group_ * nbits_, 0);
  //std::fill(arr1_[0][0], arr1_[0][0] + num_hash_ * num_group_ * (nbits_ + 1),
  //          0);
}

template <int32_t key_len, typename T, typename hash_t>
size_t ACS_Deltoid<key_len, T, hash_t>::cntNum() const {
  printf("cntnum %d\n", num_group_ * num_hash_ * nbits_);
  return num_group_ * num_hash_ * nbits_;
}


} // namespace OmniSketch::Sketch