#ifndef ATOMIC_LIGHT_SPINLOCK
#define ATOMIC_LIGHT_SPINLOCK

#include <atomic>

class spinlock {  
private: 
  std::atomic_flag lock_ = ATOMIC_FLAG_INIT; // Kept the atomic_flag private 

public: 
  spinlock() noexcept {
    lock_.clear(std::memory_order_relaxed);
  }

  void lock() noexcept {
    for (;;) {
      if (!lock_.test_and_set( std::memory_order_acquire)) {
        // if test_and_set returns false, it means the flag was previously clear
        // and we have now set it.  Lock acquired.
        return;
      }
      // Wait for lock to be released without generating cache misses
      while (lock_.test(std::memory_order_relaxed)) {
        // Spin while the flag is set (lock is held)
        // Issue X86 PAUSE or ARM YIELD instruction
        __builtin_ia32_pause();
      }
    }
  }

  bool try_lock() noexcept {
    //When you call try_lock(), the thread will not wait if the lock is already held.
    // It will immediately return, indicating whether it acquired the lock or not.
    // First do a relaxed load to check if lock is free in order to prevent
    // unnecessary cache misses if someone does while(!try_lock())
    if (!lock_.test(std::memory_order_relaxed)){
      // if test() returns false, it means that that the flag is clear(likely unlocked)
      if(!lock_.test_and_set(std::memory_order_acquire)){
        // try to acquire the lock.
        return true;
      }
    }
    return false; // Lock was either already set or we failed to set it.
  }

  void unlock() noexcept {
    lock_.clear(std::memory_order_release);
    // realese flag
  }
};

#endif //ATOMIC_LIGHT_SPINLOCK
