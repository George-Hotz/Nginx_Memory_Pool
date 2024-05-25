#ifndef _NGX_LOCK_H_INCLUDED_
#define _NGX_LOCK_H_INCLUDED_

#include "ngx_core.h"

class ngx_lock{
public:
    ngx_lock(): flag(ATOMIC_FLAG_INIT) {}
    ~ngx_lock() = default;

    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield(); // 提示操作系统进行线程切换
        }
    }
    
    void unlock() {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag;
};

#endif