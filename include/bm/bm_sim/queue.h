/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

//! @file queue.h

#ifndef BM_BM_SIM_QUEUE_H_
#define BM_BM_SIM_QUEUE_H_

#include <bm/bm_sim/logger.h>

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <ctime>

namespace bm {

/* TODO(antonin): implement non blocking read behavior */

//! A utility queueing class made available to all targets. It could be used
//! -for example- to queue packets between an ingress thread and an egress
//! thread. This is a very simple class, which does not implement anything fancy
//! (e.g. rate limiting, priority queueing, fair scheduling, ...) but can be
//! used as a base class to build something more advanced.
//! Queue includes a mutex and is thread-safe.
template <class T>
class Queue {
 public:
  //! Implementation behavior when an item is pushed to a full queue
  enum WriteBehavior {
    //! block and wait until a slot is available
    WriteBlock,
    //! return immediately
    WriteReturn
  };
  //! Implementation behavior when an element is popped from an empty queue
  enum ReadBehavior {
    //! block and wait until the queue becomes non-empty
    ReadBlock,
    //! not implemented yet
    ReadReturn
  };

 public:
  Queue()
    : capacity(1024), wb(WriteBlock), rb(ReadBlock) { }

  //! Constructs a queue with specified \p capacity and read / write behaviors
  Queue(size_t capacity,
        WriteBehavior wb = WriteBlock, ReadBehavior rb = ReadBlock)
    : capacity(capacity), wb(wb), rb(rb) {}

  //! Makes a copy of \p item and pushes it to the front of the queue
  void push_front(const T &item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_full()) {
      if (wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(item);
    lock.unlock();
    q_not_empty.notify_one();
  }

  //! Moves \p item to the front of the queue
  void push_front(T &&item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_full()) {
      if (wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(std::move(item));
    lock.unlock();
    q_not_empty.notify_one();
  }

  //! Pops an element from the back of the queue: moves the element to `*pItem`.
  void pop_back(T* pItem) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_empty())
      q_not_empty.wait(lock);
    *pItem = std::move(queue.back());
    queue.pop_back();
    lock.unlock();
    q_not_full.notify_one();
  }

  //! Get queue occupancy
  size_t size() const {
    std::unique_lock<std::mutex> lock(q_mutex);
    return queue.size();
  }

  //! Change the capacity of the queue
  void set_capacity(const size_t c) {
    // change capacity but does not discard elements
    std::unique_lock<std::mutex> lock(q_mutex);
    capacity = c;
  }

  //! Deleted copy constructor
  Queue(const Queue &) = delete;
  //! Deleted copy assignment operator
  Queue &operator =(const Queue &) = delete;

  //! Deleted move constructor (class includes mutex)
  Queue(Queue &&) = delete;
  //! Deleted move assignment operator (class includes mutex)
  Queue &&operator =(Queue &&) = delete;

 private:
  bool is_not_empty() const { return queue.size() > 0; }
  bool is_not_full() const { return queue.size() < capacity; }

  size_t capacity;
  std::deque<T> queue;
  WriteBehavior wb;
  ReadBehavior rb;

  mutable std::mutex q_mutex;
  mutable std::condition_variable q_not_empty;
  mutable std::condition_variable q_not_full;
};

template <class T>
class Advqueue {
 public:
  //! Implementation behavior when an item is pushed to a full queue
  enum WriteBehavior {
    //! block and wait until a slot is available
    WriteBlock,
    //! return immediately
    WriteReturn
  };
  //! Implementation behavior when an element is popped from an empty queue
  enum ReadBehavior {
    //! block and wait until the queue becomes non-empty
    ReadBlock,
    //! not implemented yet
    ReadReturn
  };

 public:
  Advqueue()
    : capacity(1024), wb(WriteBlock), rb(ReadBlock) {
      std::srand(std::time(nullptr));
    }

  //! Constructs a queue with specified \p capacity and read / write behaviors
  Advqueue(size_t capacity,
        WriteBehavior wb = WriteBlock, ReadBehavior rb = ReadBlock)
    : capacity(capacity), wb(wb), rb(rb) {
      std::srand(std::time(nullptr));
    }

  //! Makes a copy of \p item and pushes it to the front of the queue
  void push_front_org(const T &item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_full()) {
      if (wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(item);
    lock.unlock();
    q_not_empty.notify_one();
  }

  //! Moves \p item to the front of the queue
  void push_front_org(T &&item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_full()) {
      if (wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(std::move(item));
    lock.unlock();
    q_not_empty.notify_one();
  }

  // flag = 0, buffer the packet, flag = 1, normal packet
  void push_front(const T &item, bool flag=true) {
    std::unique_lock<std::mutex> lock(q_mutex);
    if (flag) {
      while (!is_not_full()) {
        if (wb == WriteReturn) return;
        q_not_full.wait(lock);
      }
      queue.push_front(std::move(item));
      //BMLOG_ERROR("push packet org");
      //std::cout << "org push " << item << std::endl;
    }
    else{
      while (!is_not_full_buf()) {
        if (wb == WriteReturn) return;
        q_not_full_buf.wait(lock);
      }
      queue_buf.push_front(std::move(item));
      //BMLOG_ERROR("push packet buffer");
      //std::cout << "buf push " << item << std::endl;
    }
    lock.unlock();
    //BMLOG_ERROR("push unlock");
    q_not_empty.notify_one();
  }

  void push_front(T &&item, bool flag=true) {
    std::unique_lock<std::mutex> lock(q_mutex);
    if (flag) {
      while (!is_not_full()) {
        if (wb == WriteReturn) return;
        q_not_full.wait(lock);
      }
      queue.push_front(std::move(item));
      //BMLOG_ERROR("push packet org");
      //std::cout << "org push " << item << std::endl;
    }
    else{
      while (!is_not_full_buf()) {
        if (wb == WriteReturn) return;
        q_not_full_buf.wait(lock);
      }
      queue_buf.push_front(std::move(item));
      //BMLOG_ERROR("push packet buffer");
      //std::cout << "buf push " << item << std::endl;
    }
    lock.unlock();
    //BMLOG_ERROR("push unlock");
    q_not_empty.notify_one();
  }

  void pop_back(T* pItem) {
    //BMLOG_ERROR("pop start");
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_empty_all()){
      //BMLOG_ERROR("lock loop");
      q_not_empty.wait(lock);
    }


    //BMLOG_ERROR("initital pop packet");
    //std::cout << "clock: " << std::clock_t() << std::endl;
    if ((std::rand() % 10) == 0) {
      //BMLOG_ERROR("initital pop packet buffer");
      if (is_not_empty_buf()){
          //BMLOG_ERROR("buffer not empty");
        *pItem = std::move(queue_buf.back());
        queue_buf.pop_back();
        //BMLOG_ERROR("pop packet buffer");
        //std::cout << "buf pop " << *pItem << std::endl;
      }
    }
    else{
      //BMLOG_ERROR("initital pop packet org");
      if (is_not_empty()){
        *pItem = std::move(queue.back());
        queue.pop_back();
        //BMLOG_ERROR("pop packet org");
        //std::cout << "org pop " << *pItem << std::endl;
      }
    }
    lock.unlock();
    //BMLOG_ERROR("pop unlock");
    q_not_full.notify_one();
  }


  //! Pops an element from the back of the queue: moves the element to `*pItem`.
  void pop_back_org(T* pItem) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_empty())
      q_not_empty.wait(lock);
    *pItem = std::move(queue.back());
    queue.pop_back();
    lock.unlock();
    q_not_full.notify_one();
  }

  //! Get queue occupancy
  size_t size() const {
    std::unique_lock<std::mutex> lock(q_mutex);
    return queue.size();
  }

  //! Change the capacity of the queue
  void set_capacity(const size_t c) {
    // change capacity but does not discard elements
    std::unique_lock<std::mutex> lock(q_mutex);
    capacity = c;
  }

  //! Deleted copy constructor
  Advqueue(const Advqueue &) = delete;
  //! Deleted copy assignment operator
  Advqueue &operator =(const Advqueue &) = delete;

  //! Deleted move constructor (class includes mutex)
  Advqueue(Advqueue &&) = delete;
  //! Deleted move assignment operator (class includes mutex)
  Advqueue &&operator =(Advqueue &&) = delete;

private:
  bool is_not_empty() const { return queue.size() > 0; }
  bool is_not_full() const { return queue.size() < capacity; }
  bool is_not_empty_buf() const { return queue_buf.size() > 0; }
  bool is_not_full_buf() const { return queue_buf.size() < capacity; }
  bool is_not_empty_all() const { return queue.size() + queue_buf.size() > 0; }
  bool is_not_full_all() const { return (queue.size() < capacity) && (queue_buf.size() < capacity); }

  size_t capacity;
  std::deque<T> queue;
  std::deque<T> queue_buf;
  WriteBehavior wb;
  ReadBehavior rb;

  mutable std::mutex q_mutex;
  mutable std::condition_variable q_not_empty;
  mutable std::condition_variable q_not_full;
  mutable std::condition_variable q_not_full_buf;
};

}  // namespace bm

#endif  // BM_BM_SIM_QUEUE_H_
