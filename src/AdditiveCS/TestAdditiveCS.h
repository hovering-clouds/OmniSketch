/**
 * @file AdditiveCSTest.h
 * @author hc (you@domain.com)
 * @brief Test Additive Counter Shaing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <common/test.h>
#include <sketch/ACS_CMSketch.h>

#define ACS_CONFIG_PATH "ACSCM.para"
#define ACS_CM_TEST_PATH "ACSCM.CM.test"
#define ACS_CM_PARA_PATH "ACSCM.CM.data"
using OmniSketch::Counter::ACScounter;

namespace OmniSketch::Test {

/**
 * @brief Testing class for ACS
 *
 */
class AdditiveCSTest {
private:
  const std::string_view config_file;

public:
  AdditiveCSTest(const std::string_view config_file_): config_file(config_file_){}
  ~AdditiveCSTest(){};
  void runTest();
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

void AdditiveCSTest::runTest() {
  typedef int32_t T;
  constexpr int32_t key_len = 13;
  /**
   * @brief shorthand for convenience
   *
   */
  using StreamData = Data::StreamData<key_len>;

  /// Part I.
  ///   Parse the config file
  ///
  /// Step i.  First we list the variables to parse, namely:
  ///
  int32_t depth, width;  // sketch config
  std::string data_file; // data config
  toml::array arr;       // shortly we will convert it to format
  /// Step ii. Open the config file
  Util::ConfigParser parser(config_file);
  if (!parser.succeed()) {
    return;
  }
  /// Step iii. Set the working node of the parser.
  parser.setWorkingNode(
      ACS_CM_PARA_PATH); // do not forget to to enclose it with braces
  /// Step iv. Parse num_bits and num_hash
  if (!parser.parseConfig(depth, "depth"))
    return;
  if (!parser.parseConfig(width, "width"))
    return;
  /// Step v. Move to the data node
  parser.setWorkingNode(ACS_CM_PARA_PATH);
  /// Step vi. Parse data and format
  if (!parser.parseConfig(data_file, "data"))
    return;
  if (!parser.parseConfig(arr, "format"))
    return;
  Data::DataFormat format(arr); // conver from toml::array to Data::DataFormat
  /// [Optional] User-defined rules
  ///
  /// Step vii. Parse Cnt Method.
  std::string method;
  Data::CntMethod cnt_method = Data::InLength;
  if (!parser.parseConfig(method, "cnt_method"))
    return;
  if (!method.compare("InPacket")) {
    cnt_method = Data::InPacket;
  }

  /// Part II.
  ///   Prepare sketch and data
  ///
  /// Step i. Initialize a sketch
  width = Util::NextPrime(width);
  ACScounter<T> counter(depth*width,100000,8);
  std::unique_ptr<Sketch::SketchBase<key_len, T>> ptr(
      new Sketch::ACS_CMSketch<key_len, T, OmniSketch::Hash::AwareHash>(depth, width, 0, counter));
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it

  /// Step ii. Get ground truth
  ///
  ///       1. read data
  StreamData data(data_file, format); // specify both data file and data format
  if (!data.succeed())
    return;
  Data::GndTruth<key_len, T> gnd_truth;
  gnd_truth.getGroundTruth(data.begin(), data.end(), cnt_method);
  ///       2. [optional] show data info
  fmt::print("DataSet: {:d} records with {:d} keys ({})\n", data.size(),
             gnd_truth.size(), data_file);
  /// Step iii. Insert the samples and then look up all the flows
  ///
  ///        1. update records into the sketch
  //this->testUpdate(ptr, data.begin(), data.end(),
  //                 cnt_method); // metrics of interest are in config file
  ///        2. query for all the flowkeys
  //this->testQuery(ptr, gnd_truth); // metrics of interest are in config file
  ///        3. size
  //this->testSize(ptr);
  ///        3. show metrics
  //this->show();

  return;
}

} // namespace OmniSketch::Test

#undef ACS_CONFIG_PATH
#undef ACS_CM_TEST_PATH
#undef ACS_CM_PARA_PATH
