/**
 * @file test_ACS.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_ACS
#include "test_factory.h"
#include <common/ACScounter.h>
using OmniSketch::Counter::ACScounter;

void TestInit() {
  // test constructor
  ACScounter<int32_t> ac = ACScounter<int32_t>(12,9,2);
  VERIFY(ac.N==12);
  VERIFY(ac.M==9);
  VERIFY(ac.gpnum[0]==4);
  VERIFY(ac.gpnum[1]==5);
  VERIFY(ac.cumnum[0]==0);
  VERIFY(ac.cumnum[1]==4);
  VERIFY(ac.cumnum[2]==9);
  VERIFY(ac.counter[5]==0);
  VERIFY(ac.shared_cnt==NULL);
  ACScounter<int32_t> ac2 = ACScounter<int32_t>(256,120,6);
  VERIFY(ac2.M==161);
  VERIFY(ac2.gpnum[0]==20);
  VERIFY(ac2.gpnum[1]==21);
  VERIFY(ac2.gpnum[2]==23);
  VERIFY(ac2.gpnum[3]==29);
  VERIFY(ac2.gpnum[4]==31);
  VERIFY(ac2.gpnum[5]==37);
  VERIFY(ac2.counter[97]==0);
  // test init_Restore()
  ac2.initRestore();
  VERIFY(ac2.shared_cnt!=NULL);
  VERIFY(ac2.shared_cnt[ac2.cumnum[0]+15]==13);
  VERIFY(ac2.shared_cnt[ac2.cumnum[0]+16]==12);
  VERIFY(ac2.shared_cnt[ac2.cumnum[1]+3]==13);
  VERIFY(ac2.shared_cnt[ac2.cumnum[1]+4]==12);
  VERIFY(ac2.shared_cnt[ac2.cumnum[2]+2]==12);
  VERIFY(ac2.shared_cnt[ac2.cumnum[2]+3]==11);
  VERIFY(ac2.shared_cnt[ac2.cumnum[3]+23]==9);
  VERIFY(ac2.shared_cnt[ac2.cumnum[3]+24]==8);
  VERIFY(ac2.shared_cnt[ac2.cumnum[4]+7]==9);
  VERIFY(ac2.shared_cnt[ac2.cumnum[4]+8]==8);
  VERIFY(ac2.shared_cnt[ac2.cumnum[5]+33]==7);
  VERIFY(ac2.shared_cnt[ac2.cumnum[5]+34]==6);
  std::cout << "pass test_init" << std::endl;
}

void TestGetLargeId() {
  ACScounter<int32_t> ac = ACScounter<int32_t>(256,120,6);
  // we update virtual counter 0,101,202 in each group by 10
  // so each of them should be considered as large counter
  ac.setCounter(ac.cumnum[0]+0,10);
  ac.setCounter(ac.cumnum[0]+1,10);
  ac.setCounter(ac.cumnum[0]+2,10);
  ac.setCounter(ac.cumnum[1]+0,10);
  ac.setCounter(ac.cumnum[1]+17,10);
  ac.setCounter(ac.cumnum[1]+13,10);
  ac.setCounter(ac.cumnum[2]+0,10);
  ac.setCounter(ac.cumnum[2]+9,10);
  ac.setCounter(ac.cumnum[2]+18,10);
  ac.setCounter(ac.cumnum[3]+0,10);
  ac.setCounter(ac.cumnum[3]+14,10);
  ac.setCounter(ac.cumnum[3]+28,10);
  ac.setCounter(ac.cumnum[4]+0,10);
  ac.setCounter(ac.cumnum[4]+8,10);
  ac.setCounter(ac.cumnum[4]+16,10);
  ac.setCounter(ac.cumnum[5]+0,10);
  ac.setCounter(ac.cumnum[5]+27,10);
  ac.setCounter(ac.cumnum[5]+17,10);
  std::vector<int32_t> lst;
  ac.getLargeId(lst, 0.1, ACScounter<int32_t>::GetIdMethod::theta);
  std::sort(lst.begin(),lst.end());
  VERIFY(lst.size()==3);
  VERIFY(lst[0]==0);
  VERIFY(lst[1]==101);
  VERIFY(lst[2]==202);
  ac.setCounter(ac.cumnum[0], 1);
  ac.setCounter(ac.cumnum[4]+8, 1);
  ac.setCounter(ac.cumnum[5]+17, 0);
  ac.getLargeId(lst, 0.1, ACScounter<int32_t>::GetIdMethod::rank);
  std::sort(lst.begin(),lst.end());
  VERIFY(lst.size()==3);
  VERIFY(lst[0]==0);
  VERIFY(lst[1]==101);
  VERIFY(lst[2]==202);
  ac.getLargeId(lst, 0.1, ACScounter<int32_t>::GetIdMethod::theta);
  VERIFY(lst.size()==0);
  std::cout << "pass test_large_id" << std::endl;
}

void TestRestore() {
  ACScounter<int32_t> ac = ACScounter<int32_t>(256,120,6);
  // we update virtual counter 0,101,202 in each group by 10
  // so each of them should be considered as large counter
  ac.setCounter(ac.cumnum[0]+0,10);
  ac.setCounter(ac.cumnum[0]+1,10);
  ac.setCounter(ac.cumnum[0]+2,10);
  ac.setCounter(ac.cumnum[1]+0,10);
  ac.setCounter(ac.cumnum[1]+17,10);
  ac.setCounter(ac.cumnum[1]+13,10);
  ac.setCounter(ac.cumnum[2]+0,10);
  ac.setCounter(ac.cumnum[2]+9,10);
  ac.setCounter(ac.cumnum[2]+18,10);
  ac.setCounter(ac.cumnum[3]+0,10);
  ac.setCounter(ac.cumnum[3]+14,10);
  ac.setCounter(ac.cumnum[3]+28,10);
  ac.setCounter(ac.cumnum[4]+0,10);
  ac.setCounter(ac.cumnum[4]+8,10);
  ac.setCounter(ac.cumnum[4]+16,10);
  ac.setCounter(ac.cumnum[5]+0,10);
  ac.setCounter(ac.cumnum[5]+27,10);
  ac.setCounter(ac.cumnum[5]+17,10);
  ac.restore();
  VERIFY(ac.query(0)==60);
  VERIFY(ac.query(101)==60);
  VERIFY(ac.query(202)==60);
  VERIFY(ac.query(1)==0);
  VERIFY(ac.query(100)==0);
  VERIFY(ac.query(200)==0);  
  VERIFY(ac.query(255)==0);  
  VERIFY(ac.query(121)==0);  
  VERIFY(ac.query(88)==0);  
  std::cout << "pass test_restore" << std::endl;
}

void TestUpdateOneGroup() {
  ACScounter<int32_t> ac(256, 256, 1);
  int32_t gnd[256];
  std::fill_n(gnd, 256, 0);
  VERIFY(ac.N==256);
  VERIFY(ac.M==256);
  VERIFY(ac.gpnum[0]==256);
  VERIFY(ac.cumnum[0]==0);
  VERIFY(ac.cumnum[1]==256);
  int round = 10000;
  for(int i = 0;i<round;++i){
    int id = rand()%256;
    int val = rand()%256;
    gnd[id]+=val;
    ac.update(id, val);
  }
  ac.restore();
  for(int i = 0;i<256;++i){
    if(gnd[i]!=ac.query(i)){
      VERIFY(false);
    }
  }
}

/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(ACS) {
  TestInit();
  TestGetLargeId();
  TestRestore();
  TestUpdateOneGroup();
}
/** @endcond */
#undef TEST_ACS