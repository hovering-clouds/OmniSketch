/**
 * @file ACSHashPipeTest.h
 * @author hc (you@domain.com)
 * @brief Testing HashPipe with ACS
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "ACSTest.h"
#include <sketch/ACS_HashPipe.h>

#define ACS_HP_PARA_PATH "ACS.HP.para"
#define ACS_HP_TEST_PATH "ACS.HP.test"
#define ACS_HP_DATA_PATH "ACS.HP.data"

namespace OmniSketch::Test {
/**
 * @brief Testing class for Bloom Filter with ACS
 *
 */
template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACSHashPipeTest : public ACSTestBase<key_len, T> {
  using TestBase<key_len, T>::config_file;

  Data::HXMethod hx_method;
  double num_heavy_hitter;

public:

  ACSHashPipeTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : ACSTestBase<key_len, T>("ACS Hash Pipe", config_file, ACS_HP_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test Bloom Filter
   * @details An overriden method
   */
  void runTest() override;
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of template methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

template <int32_t key_len, typename T, typename hash_t>
void ACSHashPipeTest<key_len, T, hash_t>::initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t depth, width; // sketch config
  std::string method;

  /// Step ii. Parse
  parser.setWorkingNode(ACS_HP_PARA_PATH);
  if (!parser.parseConfig(depth, "depth"))
    return;
  if (!parser.parseConfig(width, "width"))
    return;

  parser.setWorkingNode(ACS_HP_DATA_PATH);
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
  this->ptr = std::make_unique<Sketch::ACS_HashPipe<key_len, T, hash_t>>(depth, width, counter_num, counter);
}

template <int32_t key_len, typename T, typename hash_t>
void ACSHashPipeTest<key_len, T, hash_t>::runTest() {

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

#undef ACS_HP_PARA_PATH
#undef ACS_HP_TEST_PATH
#undef ACS_HP_DATA_PATH
