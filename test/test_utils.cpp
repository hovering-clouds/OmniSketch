/**
 * @file test_utils.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "test_factory.h"
#include <common/utils.h>

void TestExtendedGCD() {
  using OmniSketch::Util::ExtendedGCD;

  int32_t m,n,x,y,gcd;
  m = 1234, n = 5678;
  gcd = ExtendedGCD(m,n,x,y);
  VERIFY(gcd==2);
  VERIFY(x==704);
  VERIFY(y==-153);

  m = 100, n = 0;
  gcd = ExtendedGCD(m,n,x,y);
  VERIFY(gcd==100);
  VERIFY(x==1);
  VERIFY(y==0);

  m = 0, n = 100;
  gcd = ExtendedGCD(m,n,x,y);
  VERIFY(gcd==100);
  VERIFY(x==0);
  VERIFY(y==1);
}

void TestIsCoprime(){
  using OmniSketch::Util::IsCoprime;

  int32_t m,n;
  m = 1234, n = 5678;
  VERIFY(!IsCoprime(m,n));
  m = 123, n = 454;
  VERIFY(IsCoprime(m,n));
  m = 0, n = 12;
  VERIFY(!IsCoprime(m,n));
}

void TestMulInverse(){
  using OmniSketch::Util::MulInverse;
  VERIFY(MulInverse(3,17)==6);
  VERIFY(MulInverse(123,454)==251);
  VERIFY(MulInverse(1232509893, 45434)==45434-19799);
}

void TestNthElem(){
  using OmniSketch::Util::getNthLargestElem;
  int a[6] = {-1,2,3,4,5,-6};
  VERIFY(getNthLargestElem(a,a+6,0)==5);
  VERIFY(getNthLargestElem(a,a+6,1)==4);
  VERIFY(getNthLargestElem(a,a+6,5)==-6);
  uint32_t b[1] = {5};
  VERIFY(getNthLargestElem(b,b+1,0)==5);
}

void TestInsertMap(){
  using OmniSketch::Util::insertCntMap;
  std::map<std::string, int32_t> mp;
  insertCntMap(mp, std::string("hh"));
  insertCntMap(mp, std::string("hello"));
  insertCntMap(mp, std::string("hh"));
  VERIFY(mp[std::string("hh")]==2);
  VERIFY(mp[std::string("hello")]==1);
}

/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(utils) {
  TestExtendedGCD();
  TestIsCoprime();
  TestMulInverse();
  TestNthElem();
  TestInsertMap();
}
/** @endcond */
