
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include "ngx_core.h"
#include "ngx_palloc.h"


Ngx_Mem_Pool::Ngx_Mem_Pool(){
    bool ret = ngx_create_pool();
    assert(ret && "ngx_create_pool failed!");
}

Ngx_Mem_Pool::~Ngx_Mem_Pool(){
    ngx_destroy_pool();
}

bool Ngx_Mem_Pool::ngx_create_pool()
{
    size_t pagesize = getpagesize();    //获取页大小

    m_pool = (ngx_pool_t*)ngx_memalign(sizeof(ngx_pool_t), NGX_POOL_ALIGNMENT);
    if (m_pool == nullptr) {
        return false;
    }

    //以页大小作为内存块的大小
    m_pool->small = (ngx_pool_small_t*)ngx_memalign(pagesize, NGX_POOL_ALIGNMENT);
    if (m_pool->small == nullptr) {
        return false;
    }

    m_pool->small->last = (u_char*)m_pool->small + sizeof(ngx_pool_small_t);
    m_pool->small->end = (u_char*)m_pool->small + pagesize;
    m_pool->small->next = nullptr;
    m_pool->small->failed = 0;

    m_pool->max = pagesize - sizeof(ngx_pool_small_t);    
    m_pool->psize = pagesize;
    
    m_pool->curr = m_pool->small;     //记录当前小内存块的地址
    m_pool->large = nullptr;
    m_pool->ref = 0;

    return true;
}


void Ngx_Mem_Pool::ngx_destroy_pool()
{
    ngx_pool_small_t    *s;
    ngx_pool_large_t    *l;

    m_pool->ref = 0;

    //释放大内存
    for (l = m_pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    //释放小内存块
    for (s = m_pool->small; s; s = s->next) {
        ngx_free(s);
    }

}


void Ngx_Mem_Pool::ngx_reset_pool()
{
    ngx_pool_small_t    *s;
    ngx_pool_large_t    *l;

    for (l = m_pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    //重置小内存块
    for (s = m_pool->small; s; s = s->next) {
        s->last = (u_char*)s + sizeof(ngx_pool_small_t);
        // s->end = (u_char*)s + m_pool->psize;
        s->failed = 0;
    }

    m_pool->ref = 0;
    m_pool->curr = m_pool->small;
    m_pool->large = nullptr;
}


void* Ngx_Mem_Pool::ngx_palloc(size_t size)
{   
    if(m_pool->ref){
        return nullptr;
    }
    m_pool->ref++;

    if (size <= m_pool->max) {
        return ngx_palloc_small(size, 1);
    }
    return ngx_palloc_large(size);
}


void* Ngx_Mem_Pool::ngx_pnalloc(size_t size)
{
    if(m_pool->ref){
        return nullptr;
    }
    m_pool->ref++;

    if (size <= m_pool->max) {
        return ngx_palloc_small(size, 0);
    }
    return ngx_palloc_large(size);
}


void* Ngx_Mem_Pool::ngx_pcalloc(size_t size)
{
    void *p;
    p = ngx_palloc(size);
    if (p) {
        ngx_memzero(p, size);
    }
    return p;
}


inline void* Ngx_Mem_Pool::ngx_memalign(size_t size, size_t alignment)
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


inline void* Ngx_Mem_Pool::ngx_palloc_small(size_t size, ngx_uint_t align)
{
    u_char              *m;
    ngx_pool_small_t    *s;

    s = m_pool->curr;

    do {
        m = s->last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (s->end - m) >= size) {    //剩余内存大小比申请的内存大，则m后移，并返回
            s->last = m + size;
            m_pool->ref--;                      //引用计数减一
            return m;
        }

        s = s->next;                            //否则表示当前内存块剩余不够，访问下一个内存块

    } while (s);

    return ngx_palloc_block(size);              //如果内存块不够用，则创建一个新的内存块
}


inline void* Ngx_Mem_Pool::ngx_palloc_block(size_t size)
{
    u_char              *m;
    size_t               psize;
    ngx_pool_small_t    *s, *s_n;

    psize = m_pool->psize;                                       
    m = (u_char*)ngx_memalign(psize, NGX_POOL_ALIGNMENT);        //新申请的内存池的首地址
    if (m == nullptr) {
        return nullptr;
    }

    s_n = (ngx_pool_small_t*) m;
    s_n->end = m + psize;
    s_n->next = nullptr;
    s_n->failed = 0;

    m += sizeof(ngx_pool_small_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    s_n->last = m + size;

    for(s = m_pool->curr; s->next; s = s->next){
        if(s->failed++ >= 5){
            m_pool->curr = s->next;
        }
    }

    s->next = s_n;
    //引用计数减一
    m_pool->ref--;                        
    
    return m;
}


inline void* Ngx_Mem_Pool::ngx_palloc_large(size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    p = malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = m_pool->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_pool_large_t*)ngx_palloc_small(sizeof(ngx_pool_large_t), 1);
    if (large == nullptr) {
        ngx_free(p);
        return nullptr;
    }

    //链表头插
    large->alloc = p;
    large->next = m_pool->large;
    m_pool->large = large;
    //引用计数减一
    m_pool->ref--;

    return p;
}


ngx_int_t Ngx_Mem_Pool::ngx_pfree(void *p)
{
    ngx_pool_large_t  *l;

    for (l = m_pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_free(l->alloc);
            l->alloc = nullptr;
            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


