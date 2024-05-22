
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include "ngx_core.h"
#include "ngx_alloc.h"


ngx_uint_t  ngx_pagesize;
ngx_uint_t  ngx_pagesize_shift;
ngx_uint_t  ngx_cacheline_size;


void *
ngx_alloc(size_t size)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "malloc(%uz) failed", size);
    }
    if(debug){
        fprintf(stderr, "malloc:(%p:%uz) failed", p, size);
    }

    return p;
}


void *
ngx_calloc(size_t size)
{
    void  *p;

    p = ngx_alloc(size);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


void *
ngx_memalign(size_t alignment, size_t size)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        fprintf(stderr, "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    if(debug){
        fprintf(stderr, "posix_memalign: %p:%uz @%uz", p, size, alignment);
    }
    return p;
}


