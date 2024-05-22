# Nginx高性能内存池组件(精简版)
* 裁剪自Nginx 1.22.1源码，裁切了其他与内存池无关的组件，
* 实现了内存池最佳实践的demo，并对比了malloc和内存池在高并发下的性能差异。

## A、高并发下传统方式的弊端
* 1、频繁的系统调用导致内核态与用户态的切换，造成性能损耗
* 2、频繁使用时增加了系统内存的碎片，降低内存使用效率
* 3、没有垃圾回收机制，容易造成内存泄漏，导致内存枯竭
* 4、内存分配与释放的逻辑在程序中相隔较远时，降低程序的稳定性

## B、解决之道
* 1、使用高性能内存管理组件
* 2、手搓内存池(这里参考Nginx)

## C、内存池如何解决弊端
* 1、高并发时系统调用频繁，降低了系统的执行效率
* √ 内存池提前预先分配大块内存，统一释放，极大的减少了malloc和 free 等函数的调用。
* 2、频繁使用时增加了系统内存的碎片，降低内存使用效率
* √内存池每次请求分配大小适度的内存块，避免了碎片的产生
* 3、没有垃圾回收机制，容易造成内存泄漏
* √在生命周期结束后统一释放内存，完全避免了内存泄露的产生
* 4、内存分配与释放的逻辑在程序中相隔较远时，降低程序的稳定性
* √在生命周期结束后统一释放内存，避免重复释放指针或释放空指针等情况


## 主要API接口
* ngx_pool_t *ngx_create_pool(size_t size);                  //创建内存池
* void ngx_destroy_pool(ngx_pool_t *pool);                   //销毁内存池
* void ngx_reset_pool(ngx_pool_t *pool);                     //重置内存池

* void *ngx_palloc(ngx_pool_t *pool, size_t size);           //分配内存对齐的内存块
* void *ngx_pnalloc(ngx_pool_t *pool, size_t size);          //分配内存未对齐的内存块
* ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);            //释放内存块

## 环境要求
* Linux
* C

## 目录树
```
.
├── src                              源代码
│   ├── main.c
│   ├── ngx_alloc.c
│   └── ngx_palloc.c
├── bin                              可执行文件
│   └── ngx_mem_pool
├── doc                              word文件
│   └── 内存池笔记.docx
├── Makefile
├── LICENSE
└── README.md
```


## 结果对比
高并发下性能提升接近100%
```bash
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/ngx_mem_pool 
use_malloc
CPU Time Used: 12.110623 seconds

George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/ngx_mem_pool 1
use_pool
CPU Time Used: 6.733660 seconds
```


[@George-Hotz](https://github.com/George-Hotz/Nginx_Memory_Pool)
