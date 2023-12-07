/**
 * @file ACSCMTest.h
 * @author hc (you@domain.com)
 * @brief Test Count Min Sketch with counter sharing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <common/test.h>
#include <sketch/ACS_CMSketch.h>

#define ACS_CM_TEST_PATH "ACS.CM.test"
#define ACS_CM_PARA_PATH "ACS.CM.para"

namespace OmniSketch::Test {

template <int32_t key_len, typename T, typename hash_t = Hash::AwareHash>
class ACSCMTest : public TestBase<key_len, T> {

  Data::StreamData<key_len>& data;
  Data::CntMethod cnt_method;
  std::unique_ptr<Sketch::SketchBase<key_len, T>> ptr;

public:

  ACSCMTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : TestBase<key_len, T>("Count Min with ACS", config_file, ACS_CM_TEST_PATH), data(data_), cnt_method(method) {}

  void initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser);
  void doUpdate();
  int32_t getCntNum();

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
int32_t ACSCMTest<key_len, T, hash_t>::getCntNum(){
  return ptr->cntNum();
}

template <int32_t key_len, typename T, typename hash_t>
void ACSCMTest<key_len, T, hash_t>::doUpdate(){
  this->testUpdate(ptr, data.begin(), data.end(), cnt_method);
}

template <int32_t key_len, typename T, typename hash_t>
void ACSCMTest<key_len, T, hash_t>::initPtr(int32_t counter_num, Counter::ACScounter<T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t depth, width;
  parser.setWorkingNode(ACS_CM_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(depth, "depth"))
    return;
  if (!parser.parseConfig(width, "width"))
    return;

  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  ptr = std::make_unique<Sketch::ACS_CMSketch<key_len, T, hash_t>>(depth, width, counter_num, counter);
}

template <int32_t key_len, typename T, typename hash_t>
void ACSCMTest<key_len, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth;
  gnd_truth.getGroundTruth(data.begin(), data.end(), cnt_method);

  /// Insert the samples and then look up all the flows
  ///
  ///        1. query for all the flowkeys
  this->testQuery(ptr, gnd_truth); // metrics of interest are in config file
  ///        2. size
  this->testSize(ptr);
  ///        3. show metrics
  this->show();

  return;
}

} // namespace OmniSketch::Test

#undef ACS_CM_TEST_PATH
#undef ACS_CM_PARA_PATH
