/**
 * @file AdditiveCSTest.h
 * @author hc (you@domain.com)
 * @brief Test Additive Counter Shaing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <sketch_test/ACSTest.h>
#include <sketch_test/ACSCMTest.h>
#include <sketch_test/ACSFlowRadarTest.h>
#include <sketch_test/ACSHashPipeTest.h>
#include <sketch_test/ACSDeltoidTest.h>

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
  using Ptr = std::unique_ptr<ACSTestBase<KEYLEN, COUNTER_TYPR>>;
  const std::string_view config_file;
  Counter::ACScounter<COUNTER_TYPR> counter;
  int32_t counter_num;
  std::vector<Ptr> testPtr;

public:
  AdditiveCSTest(const std::string_view config_file_): config_file(config_file_){
    counter_num = 0;
  }
  
  void initPtr(toml::array& sketch_list, Data::StreamData<KEYLEN>& data, 
               Data::CntMethod cnt_method);

  void runTest();
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

void AdditiveCSTest::initPtr(toml::array& sketch_list, 
                             Data::StreamData<KEYLEN>& data, Data::CntMethod cnt_method){
  for(auto& node: sketch_list){
    std::string str = node.as_string()->value_or<std::string>("");
    if(str.compare("CM")==0){ // CM sketch
      testPtr.push_back(std::make_unique<ACSCMTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, data, cnt_method));
    } else if(str.compare("FR")==0){ // FlowRadar
      testPtr.push_back(std::make_unique<ACSFlowRadarTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, data, cnt_method));
    } else if(str.compare("HP")==0){ //HashPipe
      testPtr.push_back(std::make_unique<ACSHashPipeTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, data, cnt_method));
    } else if(str.compare("DT")==0){ //Deltoid
      testPtr.push_back(std::make_unique<ACSDeltoidTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, data, cnt_method));
    }
  }
}

void AdditiveCSTest::runTest() {
  srand(20231228);
  /// step i: parse ACS param
  int32_t K, ratio, iternum, clip;
  double init_val, step_val;
  std::string data_file, cmethod, gmethod;
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
  if (!parser.parseConfig(iternum, "iternum"))
    return;
  if (!parser.parseConfig(clip, "clip"))
    return;
  if (!parser.parseConfig(init_val, "init_val"))
    return;
  if (!parser.parseConfig(step_val, "step_val"))
    return;
  if (!parser.parseConfig(data_file, "data"))
    return;
  if (!parser.parseConfig(sketch_list, "sketch"))
    return;
  if (!parser.parseConfig(fmt_list, "format"))
    return;
  Data::DataFormat format(fmt_list);
  Data::CntMethod cnt_method = Data::InLength;
  Counter::GetIdMethod get_method = Counter::GetIdMethod::rank;
  if (!parser.parseConfig(cmethod, "cnt_method"))
    return;
  if (!parser.parseConfig(gmethod, "get_method"))
    return;
  if (!cmethod.compare("InPacket")) {
    cnt_method = Data::InPacket;
  }
  if (!gmethod.compare("THETA_METHOD")) {
    get_method = Counter::GetIdMethod::theta;
  }

  /// Step ii. prepare data
  Data::StreamData<KEYLEN> data(data_file, format); // specify both data file and data format
  if (!data.succeed())
    return;
  // [optional] show data info
  fmt::print("DataSet: {:d} records with xxx keys ({})\n", data.size(), data_file);
  
  /// Step iii. init sketch
  initPtr(sketch_list, data, cnt_method);
  for(auto&& ptr: testPtr){
    ptr->initPtr(counter_num, counter, parser);
    counter_num += ptr->getCntNum();
  }
  counter.initParam(counter_num, counter_num/ratio, K, 0, get_method, iternum, clip, init_val, step_val);
  for(auto&& ptr: testPtr){
    ptr->doUpdate();
  }
  counter.restore();
  /// Step iv. test sketch
  ///
  ///        1. update records into the sketch
  //this->testUpdate(ptr, data.begin(), data.end(),
  //                 cnt_method); // metrics of interest are in config file
  ///        2. query for all the flowkeys
  for(auto&& ptr: testPtr){
    ptr->runTest();
  }
  std::ofstream outf("tmp.txt", std::ios::out);
  counter.dump_results(outf);
  return;
}

} // namespace OmniSketch::Test

#undef ACS_CONFIG_PATH
