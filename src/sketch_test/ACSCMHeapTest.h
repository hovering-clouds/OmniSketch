/**
 * @file ACSCMHeapTest.h
 * @author hc (you@domain.com)
 * @brief Test Count Min Sketch with counter sharing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "ACSTest.h"
#include <sketch/ACS_CMHeap.h>

#define ACS_CMHEAP_TEST_PATH "ACS.CMHEAP.test"
#define ACS_CMHEAP_PARA_PATH "ACS.CMHEAP.para"
#define ACS_CMHEAP_DATA_PATH "ACS.CMHEAP.data"

namespace OmniSketch::Test {

template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACSCMHeapTest : public ACSTestBase<key_len, T> {

  Data::HXMethod hx_method;
  double num_heavy_hitter;

public:

  ACSCMHeapTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : ACSTestBase<key_len, T>("ACS CM Heap", config_file, ACS_CMHEAP_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test CM sketch with ACS
   * @details An overriden method
   */
  void runTest() override;
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

template <int32_t key_len, typename T, typename hash_t>
void ACSCMHeapTest<key_len, T, hash_t>::initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t depth, width, pre_thre;
  std::string method;
  parser.setWorkingNode(ACS_CMHEAP_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(depth, "depth"))
    return;
  if (!parser.parseConfig(width, "width"))
    return;
  if (!parser.parseConfig(pre_thre, "pre_thre"))
    return;

  parser.setWorkingNode(ACS_CMHEAP_DATA_PATH);
  if (!parser.parseConfig(num_heavy_hitter, "threshold_heavy_hitter"))
    return;
  hx_method = Data::TopK;
  if (!parser.parseConfig(method, "hx_method"))
    return;
  if (!method.compare("Percentile")) {
    hx_method = Data::Percentile;
  }

  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  this->ptr = std::make_unique<Sketch::ACS_CMHeap<key_len, T, hash_t>>(depth, width, pre_thre, counter_num, counter);
}

template <int32_t key_len, typename T, typename hash_t>
void ACSCMHeapTest<key_len, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth, gnd_truth_heavy_hitters;
  gnd_truth.getGroundTruth(this->data.begin(), this->data.end(), this->cnt_method);
  gnd_truth_heavy_hitters.getHeavyHitter(gnd_truth, num_heavy_hitter, hx_method);

  this->testSize(this->ptr);
  if (hx_method == Data::TopK) {
    this->testHeavyHitter(this->ptr, gnd_truth_heavy_hitters.min(), 
                          gnd_truth_heavy_hitters); // metrics of interest are in config file
  } else {
    this->testHeavyHitter(
        this->ptr, std::floor(gnd_truth.totalValue() * num_heavy_hitter + 1),
        gnd_truth_heavy_hitters); // gnd_truth_heavy_hitter: >, yet HashPipe: >=
  }
  this->show();
  
  return;
}

} // namespace OmniSketch::Test

#undef ACS_CMHEAP_TEST_PATH
#undef ACS_CMHEAP_PARA_PATH
#undef ACS_CMHEAP_DATA_PATH
