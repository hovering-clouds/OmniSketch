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
#include <sketch_test/ACSCMTest.h>
#include <sketch_test/ACSFlowRadarTest.h>

#define ACS_CONFIG_PATH "ACS.config"

#define KEYLEN 13 // 不同的key_type可以共享在一起，但是受限于实现方法暂时控制住
#define COUNTER_TYPR int32_t // 不同的counter_type不应共享在一起

namespace OmniSketch::Test {

/**
 * @brief Testing class for ACS
 *
 */
class AdditiveCSTest {
private:

  const std::string_view config_file;
  Counter::ACScounter<COUNTER_TYPR> counter;
  int32_t counter_num;

public:
  AdditiveCSTest(const std::string_view config_file_): config_file(config_file_){
    counter_num = 0;
  }
  
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
  /// step i: parse ACS param
  int32_t K, ratio;
  std::string data_file, method;
  toml::array sketch_list;
  toml::array fmt_list;
  Util::ConfigParser parser(config_file);
  if (!parser.succeed()) {
    return;
  }
  parser.setWorkingNode(ACS_CONFIG_PATH);
  if (!parser.parseConfig(K, "K"))
    return;
  if (!parser.parseConfig(ratio, "ratio"))
    return;
  if (!parser.parseConfig(data_file, "data"))
    return;
  if (!parser.parseConfig(sketch_list, "sketch"))
    return;
  if (!parser.parseConfig(fmt_list, "format"))
    return;
  Data::DataFormat format(fmt_list);
  Data::CntMethod cnt_method = Data::InLength;
  if (!parser.parseConfig(method, "cnt_method"))
    return;
  if (!method.compare("InPacket")) {
    cnt_method = Data::InPacket;
  }

  /// Step ii. prepare data
  Data::StreamData<KEYLEN> data(data_file, format); // specify both data file and data format
  if (!data.succeed())
    return;
  Data::GndTruth<KEYLEN, COUNTER_TYPR> gnd_truth;
  gnd_truth.getGroundTruth(data.begin(), data.end(), cnt_method);
  // [optional] show data info
  fmt::print("DataSet: {:d} records with {:d} keys ({})\n", data.size(),
             gnd_truth.size(), data_file);
  
  /// Step iii. init sketch
  //auto cmptr = std::make_unique<ACSCMTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, parser, data, gnd_truth);
  auto frptr = std::make_unique<ACSFlowRadarTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, parser, data, gnd_truth);
  //cmptr->initPtr(counter_num, counter);
  //counter_num += cmptr->getCntNum();
  frptr->initPtr(counter_num, counter);
  counter_num += frptr->getCntNum();

  counter.initParam(counter_num, counter_num/ratio, K);
  //cmptr->doUpdate(cnt_method);
  frptr->doUpdate(cnt_method);
  counter.restore();
  /// Step iv. test sketch
  ///
  ///        1. update records into the sketch
  //this->testUpdate(ptr, data.begin(), data.end(),
  //                 cnt_method); // metrics of interest are in config file
  ///        2. query for all the flowkeys
  //cmptr->runTest();
  frptr->runTest();
  return;
}

} // namespace OmniSketch::Test

#undef ACS_CONFIG_PATH
