
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include "ngx_core.h"
#include "ngx_palloc.h"


ngx_uint_t  ngx_pagesize;
ngx_uint_t  ngx_pagesize_shift;
ngx_uint_t  ngx_cacheline_size;

static void* ngx_memalign(size_t alignment, size_t size);


Ngx_Mem_Pool::Ngx_Mem_Pool(){
    ngx_create_pool();
}

Ngx_Mem_Pool::~Ngx_Mem_Pool(){
    ngx_destroy_pool();
}

void Ngx_Mem_Pool::ngx_create_pool()
{
    size_t size = 4096;
    m_pool = (ngx_pool_t*)ngx_memalign(NGX_POOL_ALIGNMENT, size);
    if (m_pool == nullptr) {
        return;
    }
    m_pool->ref = 0;
    m_pool->d.last = (u_char *) m_pool + sizeof(ngx_pool_t);
    m_pool->d.end = (u_char *) m_pool + size;
    m_pool->d.next = nullptr;
    m_pool->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
    m_pool->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;    //限定最大内存块大小， 大于max则分配large内存块

    m_pool->current = m_pool;
    m_pool->large = nullptr;

}


void Ngx_Mem_Pool::ngx_destroy_pool()
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;

    m_pool->ref = 0;
    //释放大内存
    for (l = m_pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }
    //释放内存块(小于一个页，4KB)
    for (p = m_pool, n = m_pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == nullptr) {
            break;
        }
    }
}


void Ngx_Mem_Pool::ngx_reset_pool()
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = m_pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    for (p = m_pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    m_pool->ref = 0;
    m_pool->current = m_pool;
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

inline void* Ngx_Mem_Pool::ngx_palloc_small(size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;

    p = m_pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {    //剩余内存大小比申请的内存大，则m后移，并返回
            p->d.last = m + size;

            m_pool->ref--;                        //引用计数减一

            return m;
        }

        p = p->d.next;                            //否则表示当前内存块剩余不够，访问下一个内存块

    } while (p);

    return ngx_palloc_block(size);         //如果内存块不够用，则创建一个新的内存块
}


inline void* Ngx_Mem_Pool::ngx_palloc_block(size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *New;

    psize = (size_t) (m_pool->d.end - (u_char *) m_pool);             //整个ngx_pool_t的容量 

    m = (u_char*)ngx_memalign(NGX_POOL_ALIGNMENT, psize);       //新申请的内存池的首地址
    if (m == nullptr) {
        return nullptr;
    }

    New = (ngx_pool_t *) m;

    New->d.end = m + psize;
    New->d.next = nullptr;
    New->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    New->d.last = m + size;

    for (p = m_pool->current; p->d.next; p = p->d.next) {         //申请新内存块，则代表链表上都申请失败，判断次数是否超过5，然后failed++
        if (p->d.failed++ > 4) {                                  //如果failed失败次数超过5，则跳过该内存块，保证链表的从current到末尾长度不超过5
            m_pool->current = p->d.next;
        }
    }

    p->d.next = New;
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
            if(debug){
                fprintf(stderr, "free: %p", l->alloc);
            }
            ngx_free(l->alloc);
            l->alloc = nullptr;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}



static void* ngx_memalign(size_t alignment, size_t size)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        fprintf(stderr, "posix_memalign(%uz, %uz) failed", alignment, size);
        p = nullptr;
    }

    if(debug){
        fprintf(stderr, "posix_memalign: %p:%uz @%uz", p, size, alignment);
    }
    return p;
}