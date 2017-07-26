/** Ranges -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2017, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 *
 * @section Description
 *
 * @author Donald Nguyen <ddn@cs.texas.edu>
 * @author Loc Hoang <l_hoang@utexas.edu> (class SpecificRange)
 */
#ifndef GALOIS_RUNTIME_RANGE_H
#define GALOIS_RUNTIME_RANGE_H

#include "Galois/gstl.h"
#include "Galois/Substrate/ThreadPool.h"

#include <iterator>

namespace Galois {
namespace Runtime {

extern unsigned int activeThreads;

// TODO(ddn): update to have better forward iterator behavor for blocked/local iteration

template<typename T>
class LocalRange {
  T* container;

public:
  typedef T container_type;
  typedef typename T::iterator iterator;
  typedef typename T::local_iterator local_iterator;
  typedef iterator block_iterator;
  typedef typename std::iterator_traits<iterator>::value_type value_type;
  
  LocalRange(T& c): container(&c) { }

  iterator begin() const { return container->begin(); }
  iterator end() const { return container->end(); }

  // TODO fix constness of local containers
  /* const */ T& get_container() const { return *container; }

  std::pair<block_iterator, block_iterator> block_pair() const {
    return Galois::block_range(begin(), end(), Substrate::ThreadPool::getTID(), activeThreads);
  }

  std::pair<local_iterator, local_iterator> local_pair() const {
    return std::make_pair(container->local_begin(), container->local_end());
  }

  local_iterator local_begin() const { return container->local_begin(); }
  local_iterator local_end() const { return container->local_end(); }

  block_iterator block_begin() const { return block_pair().first; }
  block_iterator block_end() const { return block_pair().second; }
};

template<typename T>
inline LocalRange<T> makeLocalRange(T& obj) { return LocalRange<T>(obj); }

template<typename IterTy>
class StandardRange {
  IterTy ii, ei;

public:
  typedef IterTy iterator;
  typedef iterator local_iterator;
  typedef iterator block_iterator;

  typedef typename std::iterator_traits<IterTy>::value_type value_type;

  StandardRange(IterTy b, IterTy e): ii(b), ei(e) { }

  iterator begin() const { return ii; }
  iterator end() const { return ei; }

  std::pair<block_iterator, block_iterator> block_pair() const {
    return Galois::block_range(ii, ei, Substrate::ThreadPool::getTID(), activeThreads);
  }

  std::pair<local_iterator, local_iterator> local_pair() const {
    return block_pair();
  }

  local_iterator local_begin() const { return block_begin(); }
  local_iterator local_end() const { return block_end(); }

  block_iterator block_begin() const { return block_pair().first; }
  block_iterator block_end() const { return block_pair().second; }
};

template<typename IterTy>
inline StandardRange<IterTy> makeStandardRange(IterTy begin, IterTy end) {
  return StandardRange<IterTy>(begin, end);
}

/** 
 * SpecificRange is a range type where a threads range is specified by
 * an an int array that tells you where each thread should begin its
 * iteration 
 */
template<typename IterTy>
class SpecificRange {
  IterTy global_begin, global_end;
  const uint32_t* thread_beginnings;

public:
  typedef IterTy iterator;
  typedef iterator local_iterator;
  typedef iterator block_iterator;

  typedef typename std::iterator_traits<IterTy>::value_type value_type;

  SpecificRange(IterTy b, IterTy e, const uint32_t* thread_ranges): 
    global_begin(b), global_end(e), thread_beginnings(thread_ranges) { }

  iterator begin() const { return global_begin; }
  iterator end() const { return global_end; }

  /* Using the thread_beginnings array which tells you which node each thread
   * should begin at, we can get the local block range for a particular 
   * thread 
   *
   * @returns A pair of iterators that specifies the beginning and end
   * of the range for this particular thread.
   */
  std::pair<block_iterator, block_iterator> block_pair() const {
    uint32_t my_thread_id = Substrate::ThreadPool::getTID();

    iterator local_start = global_begin + thread_beginnings[my_thread_id];
    iterator local_end = global_begin + thread_beginnings[my_thread_id + 1];

    //printf("thread %u gets start %u and end %u\n", my_thread_id, *local_start, 
    //       *local_end);

    return std::make_pair(local_start, local_end);
  }

  std::pair<local_iterator, local_iterator> local_pair() const {
    return block_pair();
  }

  local_iterator local_begin() const { return block_begin(); }
  local_iterator local_end() const { return block_end(); }

  block_iterator block_begin() const { return block_pair().first; }
  block_iterator block_end() const { return block_pair().second; }
};

/** * Creates a SpecificRange object.
 *
 * @tparam IterTy The iterator type used by the range object
 * @param begin The global beginning of the range
 * @param end The global end of the range
 * @param thread_ranges An array of iterators that specifies where each
 * thread's range begins
 * @returns A SpecificRange object 
 */
template<typename IterTy>
inline SpecificRange<IterTy> makeSpecificRange(IterTy begin, 
                                               IterTy end,
                                               const uint32_t* thread_ranges) {
  return SpecificRange<IterTy>(begin, end, thread_ranges);
}

}
} // end namespace Galois
#endif
