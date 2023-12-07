/**
 * @file ACScounter1.h
 * @author hc (you@domain.com)
 * @brief Counter type for additive counter sharing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#define THETA_METHOD
#define DEBUG_ACS
#include "utils.h"
#include <numeric>
#include <set>

namespace OmniSketch::Counter{

/**
 * @brief Integer of fixed length
 * 
 * @details Counter is always interpreted as a *non-negative* value. When overflow occurs, the value will
 * be set as -1 and later updates would be ignored.
 */
class ShadowCounter{
private:
  int32_t val;
public:
  static size_t len;
  ShadowCounter(int32_t val_=0):val(val_){}
  static void set_len(size_t len_) {len = len_;}
  bool overflow() const {
    return val==-1;
  }
  void update(int32_t v) {
    if(val==-1||val+v<0||val+v>=(1<<len)){
      val = -1;
    } else {
      val+=v;
    }
  }
  int32_t query(){
    return val;
  }
};

/**
 * @brief Additive Counter Sharing.
 * @note All counter values stored inside are considered as NON-NEGATIVE values.
 * 
 * @tparam T inner counter type
 */
template <typename T>
class ACScounter {
#ifdef TEST_ACS
public:
#else
private:
#endif
  bool is_initialized; // if parameters has been initialized
  bool restore_inited; // if restored_counter has been allocated
  bool use_shadow; // if use shadow counter
  int32_t N; // number of virtual counter
  int32_t M; // number of physical counter
  int32_t K; // number of group
  uint32_t update_cnt; // used as a 'pseudo random number' to select which counter to update
  uint32_t unrestored; // number of unrestored counter, i.e. sum of shared_cnt
  int32_t* gpnum; // counter number in each group
  int32_t* cumnum; // cumulative sum of `gpnum`, start from 0
  T* counter; // real counter array of type T
  ShadowCounter* shadow; // shadow counter
  
  // The following data structures are used only in restore phase, i.e. in control plane
  int32_t* shared_cnt; // track how many virtual counters are sharing the physical counter
  T* restored_value; // the restored value of virtual counters
  bool* is_restored; // whether the given counter has been restored 


  ACScounter(const ACScounter &) = delete;
  ACScounter(ACScounter &&) = delete;

  // how we decide whether a counter should be considered as large
  // used in `getLargeId`
  enum class GetIdMethod{theta, rank};

  void initRestore();
  void preShadow();
  void postShadow();
  void setCounter(int32_t inner_idx, T val);
  /**
   * @brief Get (virtual) ids of candidate large counter
   * 
   * @param id_list vector to store the result, old values would be cleared
   * @param tr theta or rank, according to `method`
   * @param method how we decide whether a counter is large
   */
  void getLargeId(std::vector<int32_t>& id_list, double tr, GetIdMethod method = GetIdMethod::theta);
  void restore_large(const std::vector<int32_t>& id_list, int32_t clip=0);
  void restore_small();
  

public:
  /**
   * @brief Construct ACScounter and calculate some config params, e.g. `gpnum`
   * 
   */
  ACScounter(int32_t n, int32_t m, int32_t k, int32_t shadow_len = 0);

  /**
   * @brief Construct an empty ACScounter array, need to initialize later 
   * 
   */
  ACScounter();
  
  /**
   * @brief release pointer
   * 
   */
  ~ACScounter();

  /**
   * @brief initialize empty counter array, do exactly as the Constructor method 
   * 
   */
  void initParam(int32_t n, int32_t m, int32_t k, int32_t shadow_len = 0);

  /**
   * @brief update counter
   * 
   * @param idx virtual counter id
   * @param val value
   */
  void update(int32_t idx, T val);

  void restore();

  T query(int32_t idx);

  T& operator[](size_t idx);

  /**
   * @brief clear the counters (won't clear restored value)
   * 
   */
  void clear();
  /**
   * @brief dump restored counters
   * 
   */
  void dump_results();
  void query_map_values(int32_t idx);
};

} //namespace Ominisketch
namespace OmniSketch::Counter {

size_t ShadowCounter::len;

template <typename T>
ACScounter<T>::ACScounter(int32_t n, int32_t m, int32_t k, int32_t shadow_len)
  :update_cnt(0), shared_cnt(NULL), restored_value(NULL), is_restored(NULL), shadow(NULL) {
  use_shadow = false;
  is_initialized = false;
  restore_inited = false;
  initParam(n,m,k,shadow_len);
}

template <typename T>
ACScounter<T>::ACScounter()
  :update_cnt(0), shared_cnt(NULL), restored_value(NULL), is_restored(NULL), shadow(NULL) {
  use_shadow = false;
  is_initialized = false;
  restore_inited = false;
}

template <typename T>
void ACScounter<T>::initParam(int32_t n, int32_t m, int32_t k, int32_t shadow_len){
  assert(!is_initialized);
  is_initialized = true;
  N = n;
  K = k;
  gpnum = new int32_t[K];
  cumnum = new int32_t[K+1];
  // get gpnum which are pairwise coprime
  int32_t lastnum = m/K;
  int32_t round = 0;
  while (round<K){
    int i = 0;
    while(i<round){
      if(Util::IsCoprime(lastnum,gpnum[i])){
        i++;
      } else {
        lastnum++;
        i = 0;
      }
    }
    gpnum[round++]=lastnum;
  }
  // calculate cumsum
  cumnum[0] = 0;
  for(int i = 0;i<round;++i){
    cumnum[i+1] = cumnum[i]+gpnum[i];
  }
  M = cumnum[K];
  counter = new T[M];
  std::fill_n(counter, M, 0);
  if(shadow_len>0){
    use_shadow = true;
    shadow = new ShadowCounter[N]; // constructor will set 0
    ShadowCounter::set_len(shadow_len);
  }
}

template <typename T>
ACScounter<T>::~ACScounter(){
  delete[] gpnum;
  delete[] cumnum;
  delete[] counter;
  if(restore_inited){
    delete[] shared_cnt;
    delete[] restored_value;
    delete[] is_restored;
  }
  if(use_shadow){
    delete[] shadow;
  }
}

template <typename T>
void ACScounter<T>::setCounter(int32_t inner_idx, T val){
  #ifdef DEBUG_ACS
  assert(inner_idx<M);
  #endif
  counter[inner_idx] = val;
}

template <typename T>
void ACScounter<T>::initRestore(){
  restore_inited = true;
  unrestored = N;
  shared_cnt = new int32_t[M];
  restored_value = new T[N];
  is_restored = new bool[N];
  std::fill_n(shared_cnt,M,0);
  std::fill_n(restored_value,N,0);
  std::fill_n(is_restored, N, false);
  // init shared_cnt
  for(int i = 0;i<K;++i){
    int inc = N/gpnum[i];
    int num1 = N%gpnum[i];
    std::fill(shared_cnt+cumnum[i], shared_cnt+cumnum[i]+num1, inc+1);
    std::fill(shared_cnt+cumnum[i]+num1, shared_cnt+cumnum[i+1], inc);
  }
}

template <typename T>
void ACScounter<T>::preShadow(){
  for(int32_t id = 0;id<N;++id){
    if(!shadow[id].overflow()){
      restored_value[id] = shadow[id].query();
      unrestored-=1;
      is_restored[id] = true;
      for(int32_t i = 0;i<K;++i){
        shared_cnt[cumnum[i]+id%gpnum[i]]-=1;
      }
    }
  }
  return;
}

template <typename T>
void ACScounter<T>::getLargeId(std::vector<int32_t>& id_list, double tr, GetIdMethod method){
  // here we assume type T (some numeric type) is big enough to store the sum
  if(unrestored==0){ // corner case
    id_list.clear();
    return;
  }
  T sum = std::accumulate(counter, counter+M, 0);
  double mu = sum/unrestored;
  T thre;
  std::vector<int32_t> ids, lastids;
  // for efficiency, we propose the following filter scheme:
  // 1. find the minimum n such that $\prod_{i=0}^{n} gpnum[i]$ >= N;
  // 2. use Chinese Remainder Theorem to get the candidate ids for the first n groups;
  // 3. use remained groups to check whether the ids from step 2 is truly of large counters;
  if(method==GetIdMethod::theta){
    thre = T(tr*sum/K+(mu/K)*(unrestored/gpnum[0]));
  } else {
    thre = Util::getNthLargestElem(counter, counter+cumnum[1], (size_t)(tr*gpnum[0]));
  }
  for(int j = cumnum[0]; j<cumnum[1]; ++j)
    if(counter[j]>=thre)
      lastids.push_back(j);
  int32_t i = 1, mod = gpnum[0];
  // step1&2
  while(i<K){
    int32_t g_inv, mod_inv;
    int32_t g = gpnum[i];
    g_inv = Util::MulInverse(g, mod);
    mod_inv = Util::MulInverse(mod,g);
    if(method==GetIdMethod::theta){
      thre = T(tr*sum/K+(mu/K)*(unrestored/g));
    } else {
      thre = Util::getNthLargestElem(counter+cumnum[i], counter+cumnum[i+1], (size_t)(tr*gpnum[i]));
    }
    for(int j = cumnum[i]; j<cumnum[i+1]; ++j)
      if(counter[j]>=thre){
        // get the following congruence equation:
        // (new_id % mod == id) and (new_id % g == gp_id)
        // the solution is new_id % mod*g == g*g_inv*id+mod*mod_inv*gp_id
        for(uint32_t id:lastids){
          int32_t gp_id = j-cumnum[i];
          int32_t new_id = (g*g_inv*id+mod*mod_inv*gp_id)%(mod*g);
          if(new_id<N) // only solutions that less than N are valid
            ids.push_back(new_id);
        }
      }
    ids.swap(lastids);
    ids.clear();
    i+=1;
    if(mod>N/g) {break;}
    else {mod*=g;}
  }
  // step3
  while(i<K){
    uint32_t g = gpnum[i];
    if(method==GetIdMethod::theta){
      thre = T(tr*sum/K+(mu/K)*(unrestored/g));
    } else {
      thre = Util::getNthLargestElem(counter+cumnum[i], counter+cumnum[i+1], (size_t)(tr*gpnum[i]));
    }
    for(int32_t id:lastids){
      if(counter[cumnum[i]+id%g]>=thre)
        ids.push_back(id);}
    ids.swap(lastids);
    ids.clear();
    i+=1;
  }
  // step4: remove the counter ids that has been restored
  if(is_restored!=NULL){
    for(int32_t id:lastids){
      if(!is_restored[id]){
        ids.push_back(id);
      }
    }
    ids.swap(lastids);
    ids.clear();
  }
  id_list.swap(lastids);
}

template <typename T>
void ACScounter<T>::restore_large(const std::vector<int32_t>& id_list, int32_t clip){
  // step1. restore S, which is the sum of all large counters
  std::map<int32_t, int32_t> slots; // (pcounter id, mapped times)
  int32_t num_slots = 0;
  T sum_large = 0;
  T V = std::accumulate(counter, counter+M, 0);
  for(int32_t i = 0;i<K;++i)
    for(int32_t id: id_list)
      Util::insertCntMap(slots,cumnum[i]+id%gpnum[i]);
  for(auto slot: slots){
    sum_large += counter[slot.first];
    num_slots += shared_cnt[slot.first]-slot.second;
  }
  if(sum_large<=0) return; // corner case
  T S = (sum_large-(double(num_slots)/K)*V/unrestored)/(1-double(num_slots)/(unrestored*K));
  // step2. restore large counters
  double mu_small;
  if(unrestored==id_list.size()) // corner case
    mu_small = 0;
  else
    mu_small = (V-S)/double((unrestored-id_list.size())*K);
  for(int32_t id: id_list){
    std::vector<double> allcnt;
    for(int32_t i = 0;i<K;++i){
      int32_t counter_id = cumnum[i]+id%gpnum[i];
      int32_t map_times = slots[counter_id];
      double pure_cnt = counter[counter_id]-mu_small*(shared_cnt[counter_id]-map_times);
      allcnt.push_back(pure_cnt/map_times);
    }
    std::sort(allcnt.begin(),allcnt.end());
    auto begin = allcnt.begin(), end = allcnt.end();
    for(int32_t i = 0;i<clip;++i){
      ++begin;
      --end;
    }
    restored_value[id] = T(std::accumulate(begin,end,0.0)*K/(K-2*clip));
    //printf("large %d, val %d\n", id, restored_value[id]);
  }
  // step3. subtract restored large values from `counter` array
  for(int32_t id: id_list){
    unrestored-=1;
    is_restored[id] = true;
    for(int32_t i = 0;i<K;++i){
      int32_t counter_id = cumnum[i]+id%gpnum[i];
      if(counter[counter_id]<restored_value[id]/K) // clip to lowerbound 0
        counter[counter_id] = 0;
      else
        counter[counter_id]-=restored_value[id]/K;
      shared_cnt[counter_id]-=1;
    }
  }
}

template <typename T>
void ACScounter<T>::restore_small(){
  if(unrestored==0){return;} // corner case
  T sum = std::accumulate(counter, counter+M, 0);
  double mu = double(sum)/(unrestored*K);
  for(int id = 0;id<N;++id){
    if(is_restored[id])
      continue; // has been restored
    double tmpcnt = 0;
    T min_cnt;
    for(int j = 0;j<K;++j){
      int32_t counter_id = cumnum[j]+id%gpnum[j];
      #ifdef DEBUG_ACS
      assert(shared_cnt[counter_id]>=1);
      #endif
      tmpcnt += counter[counter_id]-(shared_cnt[counter_id]-1)*mu;
      if(j==0){min_cnt = counter[counter_id];}
      min_cnt = std::min(min_cnt, counter[counter_id]);
    }
    restored_value[id] = std::min(K*min_cnt ,std::max(T(tmpcnt),0));
    unrestored-=1;
    is_restored[id] = true;
  }
  #ifdef DEBUG_ACS
  assert(unrestored==0);
  #endif
}

template <typename T>
void ACScounter<T>::postShadow(){
  int32_t addval = 1 << ShadowCounter::len;
  for(int32_t id = 0;id<N;++id){
    if(shadow[id].overflow()){
      restored_value[id] += addval;
      #ifdef DEBUG_ACS
      assert(is_restored[id]);
      #endif
    }
  }
  return;
}

//#################
// public methods #
//#################

template <typename T>
void ACScounter<T>::update(int32_t idx,T val){
  #ifdef DEBUG_ACS
  assert(idx<N);
  assert(is_initialized);
  #endif
  update_cnt++;
  if(use_shadow&&!shadow[idx].overflow()){ // update shadow counter
    shadow[idx].update(val);
  } else { // update shared counter
    int gp = update_cnt%K;
    counter[cumnum[gp]+idx%gpnum[gp]] += val;
  }
}

template <typename T>
void ACScounter<T>::restore(){
  #ifdef DEBUG_ACS
  assert(is_initialized);
  #endif
  // step1. initialize data structures
  initRestore();
  if(use_shadow)
    preShadow();
  // step2. iteratively restore large counters
  std::vector<int32_t> large_list;
  const int32_t clip = 0;

  #ifdef THETA_METHOD
  const int32_t iter_num = 2;
  const int32_t decay_ratio = 2;
  double theta = 0.1;
  for(int32_t i = 0;i<iter_num;++i){
    getLargeId(large_list, theta, GetIdMethod::theta);
    if(large_list.size()==0)
      continue;
    restore_large(large_list, clip);
    theta/=decay_ratio;
  }
  #else
  const int32_t iter_num = 20;
  const double incr_rank = 0.02;
  double rank = 0.02;
  for(int32_t i = 0;i<iter_num;++i){
    getLargeId(large_list, rank, GetIdMethod::rank);
    if(large_list.size()==0)
      continue;
    restore_large(large_list, clip);
    rank+=incr_rank;
    small_num-=large_list.size();
  }
  #endif
  
  // step3. restore small counters
  restore_small();
  if(use_shadow)
    postShadow();
}

template <typename T>
T ACScounter<T>::query(int32_t idx){
  #ifdef DEBUG_ACS
  assert(idx<N);
  assert(is_initialized);
  #endif
  return restored_value[idx];
}

template <typename T>
T& ACScounter<T>::operator[](size_t idx){
  #ifdef DEBUG_ACS
  assert(is_initialized);
  #endif
  return restored_value[idx];
}

template <typename T>
void ACScounter<T>::clear(){
  if(!is_initialized){return;}
  std::fill(counter,counter+M,0);
  if(!use_shadow){return;}
  std::fill(shadow, shadow+N, 0);
  if(!restore_inited){return;}
  std::fill(shared_cnt, shared_cnt+M, 0);
  std::fill(restored_value, restored_value+N, 0);
  std::fill(is_restored, is_restored+N, false);
}

template <typename T>
void ACScounter<T>::dump_results(){
  for(int i = 0;i<N;++i){
    printf("%d ", restored_value[i]);
    if(i%100==99)
      printf("\n");
  }
}

template <typename T>
void ACScounter<T>::query_map_values(int32_t idx){
  printf("idx: %d, ", idx);
  for(int i = 0;i<K;++i){
    printf("%d ", counter[cumnum[i]+idx%gpnum[i]]);
  }
  printf("\n");
}


}// end of namespace Counter

#undef THETA_METHOD
#undef DEBUG_ACS