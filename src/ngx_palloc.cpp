
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include "ngx_core.h"
#include "ngx_palloc.h"


ngx_pool_t* ngx_create_pool()
{
    size_t pagesize = getpagesize();    //获取页大小

    ngx_pool_t *pool = (ngx_pool_t*)ngx_memalign(sizeof(ngx_pool_t), NGX_POOL_ALIGNMENT);
    if (pool == nullptr) {
        return nullptr;
    }

    //以页大小作为内存块的大小
    pool->small = (ngx_small_t*)ngx_memalign(pagesize, NGX_POOL_ALIGNMENT);
    if (pool->small == nullptr) {
        return nullptr;
    }

    pool->small->last = (u_char*)pool->small + sizeof(ngx_small_t);
    pool->small->end = (u_char*)pool->small + pagesize;
    pool->small->next = nullptr;
    pool->small->failed = 0;

    pool->max = pagesize - sizeof(ngx_small_t);    
    pool->psize = pagesize;
    
    pool->curr = pool->small;     //记录当前小内存块的地址
    pool->large = nullptr;

    return pool;
}


void ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_small_t    *s;
    ngx_large_t    *l;

    //释放大内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    //释放小内存块
    for (s = pool->small; s; s = s->next) {
        ngx_free(s);
    }

}


void ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_small_t    *s;
    ngx_large_t    *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    //重置小内存块
    for (s = pool->small; s; s = s->next) {
        s->last = (u_char*)s + sizeof(ngx_small_t);
        // s->end = (u_char*)s + pool->psize;
        s->failed = 0;
    }

    pool->curr = pool->small;
    pool->large = nullptr;
}


void* ngx_palloc(ngx_pool_t *pool, size_t size)
{   
    void* p = nullptr;
    if (size <= pool->max) {
        p = ngx_palloc_small(pool, size, 1);
        return p;
    }
    p = ngx_palloc_large(pool, size);
    return p;
}


void* ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    void* p = nullptr;
    if (size <= pool->max) {
        p = ngx_palloc_small(pool, size, 0);
        return p;
    }
    p = ngx_palloc_large(pool, size);
    return p;
}


void* ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;
    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }
    return p;
}


inline void* ngx_memalign(size_t size, size_t alignment)
{
    void  *p;
    int    err;
    err = posix_memalign(&p, alignment, size);
    if (err) {
        fprintf(stderr, "posix_memalign(%uz, %uz) failed", alignment, size);
        p = nullptr;
    }

    return p;
}


inline void* ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char         *m;
    ngx_small_t    *s;

    s = pool->curr;

    do {
        m = s->last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (s->end - m) >= size) {    //剩余内存大小比申请的内存大，则m后移，并返回
            s->last = m + size;
            return m;
        }

        s = s->next;                            //否则表示当前内存块剩余不够，访问下一个内存块

    } while (s);

    return ngx_palloc_block(pool, size);              //如果内存块不够用，则创建一个新的内存块
}


inline void* ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char         *m;
    size_t          psize;
    ngx_small_t    *s, *s_n;

    psize = pool->psize;                                       
    m = (u_char*)ngx_memalign(psize, NGX_POOL_ALIGNMENT);        //新申请的内存池的首地址
    if (m == nullptr) {
        return nullptr;
    }

    s_n = (ngx_small_t*) m;
    s_n->end = m + psize;
    s_n->next = nullptr;
    s_n->failed = 0;

    m += sizeof(ngx_small_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    s_n->last = m + size;

    for(s = pool->curr; s->next; s = s->next){
        if(s->failed++ >= 5){
            pool->curr = s->next;
        }
    }

    s->next = s_n;                    
    
    return m;
}


inline void* ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void         *p;
    ngx_uint_t    n;
    ngx_large_t  *large;

    p = malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_large_t*)ngx_palloc_small(pool, sizeof(ngx_large_t), 1);
    if (large == nullptr) {
        ngx_free(p);
        return nullptr;
    }

    //链表头插
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


bool ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_free(l->alloc);
            l->alloc = nullptr;
            return true;
        }
    }

    return false;
}


