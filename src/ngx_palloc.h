
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_

#include "ngx_core.h"


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;
    void                 *alloc;
};


typedef struct {
    u_char               *last;            //内存块最新的地址
    u_char               *end;             //内存块末尾的地址
    ngx_pool_t           *next;            //下一块内存的地址(链表)
    ngx_uint_t            failed;          //该内存块分配失败的次数
} ngx_pool_data_t;


struct ngx_pool_s {                        //内存池的首个内存块
    ngx_pool_data_t       d;               //内存块链表
    size_t                max;             //内存块的最大容量
    ngx_pool_t           *current;         //当前所处的内存块地址(ngx_pool_s)
    ngx_pool_large_t     *large;           //大内存       
};


ngx_pool_t *ngx_create_pool(size_t size);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
