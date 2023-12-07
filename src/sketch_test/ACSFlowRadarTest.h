/**
 * @file ACSFlowRadarTest.h
 * @author hc (you@domain.com)
 * @brief Test Flow Radar with counter sharing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "ACSTest.h"
#include <sketch/ACS_FlowRadar.h>

#define ACS_FR_PARA_PATH "ACS.FlowRadar.para"
#define ACS_FR_TEST_PATH "ACS.FlowRadar.test"

namespace OmniSketch::Test {

/**
 * @brief Testing class for Flow Radar
 *
 */
template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACSFlowRadarTest : public TestBase<key_len, T> {
  using TestBase<key_len, T>::config_file;

  Data::StreamData<key_len>& data;
  Data::CntMethod cnt_method;
  std::unique_ptr<Sketch::SketchBase<key_len, T>> ptr;

public:

  ACSFlowRadarTest(const std::string_view config_file, Data::StreamData<key_len>& _data, Data::CntMethod method)
      : TestBase<key_len, T>("ACS Flow Radar", config_file, ACS_FR_TEST_PATH),  data(_data), cnt_method(method) {}

  void initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser);
  void doUpdate();
  int32_t getCntNum();

  /**
   * @brief Test Flow Radar with ACS
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
int32_t ACSFlowRadarTest<key_len, T, hash_t>::getCntNum(){
  return ptr->cntNum();
}

template <int32_t key_len, typename T, typename hash_t>
void ACSFlowRadarTest<key_len, T, hash_t>::doUpdate(){
  this->testUpdate(ptr, data.begin(), data.end(), cnt_method);
}

template <int32_t key_len, typename T, typename hash_t>
void ACSFlowRadarTest<key_len, T, hash_t>::initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t flow_filter_bit, flow_filter_hash, count_table_num,
      count_table_hash;  // sketch config
  parser.setWorkingNode(ACS_FR_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(flow_filter_bit, "flow_filter_bit"))
    return;
  if (!parser.parseConfig(flow_filter_hash, "flow_filter_hash"))
    return;
  if (!parser.parseConfig(count_table_num, "count_table_num"))
    return;
  if (!parser.parseConfig(count_table_hash, "count_table_hash"))
    return;

  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  ptr = std::make_unique<Sketch::ACS_FlowRadar<key_len, T, hash_t>>(
        flow_filter_bit, flow_filter_hash, count_table_num, count_table_hash, counter_num, counter);
}

template <int32_t key_len, typename T, typename hash_t>
void ACSFlowRadarTest<key_len, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth;
  gnd_truth.getGroundTruth(data.begin(), data.end(), cnt_method);

  this->testSize(ptr);
  this->testDecode(ptr, gnd_truth);
  this->show();

  return;
}

} // namespace OmniSketch::Test

#undef ACS_FR_PARA_PATH
#undef ACS_FR_DATA_PATH

