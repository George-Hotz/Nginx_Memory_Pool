
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_

#include "ngx_core.h"
#include "ngx_lock.h"

/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */

#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_large_t)), NGX_POOL_ALIGNMENT)
    
#define ngx_free          free



//大内存块
struct ngx_large_t {
    ngx_large_t          *next;            //链接下一个内存块
    void                 *alloc;           //内存地址
};

//小内存块(块大小相同)
struct ngx_small_t {
    u_char               *last;            //内存块最新的地址
    u_char               *end;             //内存块末尾的地址
    ngx_uint_t            failed;          //该内存块分配失败的次数
    ngx_small_t          *next;            //链接下一个内存块
};


//内存池的首个内存块
struct ngx_pool_t {
    size_t                psize;           //内存的页大小         
    size_t                max;             //内存块大小判断
    ngx_small_t          *curr;            //当前小内存块地址
    ngx_small_t          *small;           //小内存块地址
    ngx_large_t          *large;           //大内存块地址
};


class Ngx_Mem_Pool{
public:
    static Ngx_Mem_Pool& get_instance(){
        static Ngx_Mem_Pool Mem_Pool;
        return Mem_Pool;
    }

    void* ngx_palloc(size_t size);      
    void* ngx_pnalloc(size_t size);
    void* ngx_pcalloc(size_t size);

    void ngx_reset_pool();
    bool ngx_pfree(void *p);

private:
    ngx_pool_t* m_pool;
    ngx_lock spin_lock;
    
private:
    Ngx_Mem_Pool();
    ~Ngx_Mem_Pool();

    bool ngx_create_pool();
    void ngx_destroy_pool();

    inline void* ngx_memalign(size_t size, size_t alignment);
    inline void* ngx_palloc_small(size_t size, ngx_uint_t align);    //分配小内存，小于max
    inline void* ngx_palloc_block(size_t size);
    inline void* ngx_palloc_large(size_t size);                      //分配大内存，大于max

};


#endif /* _NGX_PALLOC_H_INCLUDED_ */
