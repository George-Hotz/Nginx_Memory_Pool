# Nginx高性能内存池组件
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


## 版本更新说明
* 2024/5/22（main）：完成对Nginx内存池组件的拆分，C语言版内存池
* 2024/5/23（main_v1）：由C++重构Nginx内存池，封装成类和对应接口
* 2024/5/24（main_v2）：优化Nginx内存池数据结构，解耦大小内存块分配，加入线程同步（自旋锁）


## 环境要求
* Linux
* C++11

## 目录树
```
.
├── src                              源代码
│   ├── main.cpp
│   ├── ngx_alloc.cpp
│   └── ngx_palloc.cpp
├── bin                              可执行文件
│   └── ngx_mem_pool
├── doc                              word文件
│   └── 内存池笔记.docx
├── Makefile
├── LICENSE
└── README.md
```


## 结果对比

```bash
#main版本（C语言版本）
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/ngx_mem_pool 
use_malloc
CPU Time Used: 12.110623 seconds #（使用malloc） 
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/ngx_mem_pool 1
use_pool
CPU Time Used: 6.733660 seconds #（使用内存池） 

#main_v1版本（C++版本）无线程同步
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/ngx_mem_pool 
use_malloc
CPU Time Used: 11.851856 seconds #（使用malloc） 
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/ngx_mem_pool 1
use_pool
CPU Time Used: 5.119002 seconds #（使用内存池） 

#main_v2版本（C++版本）有线程同步
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/demo1 
use_malloc
CPU Time Used: 11.516082 seconds #（使用malloc）       
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/demo1 1
use_pool
CPU Time Used: 9.020385 seconds #（使用内存池，互斥锁同步线程）
George@R9000P:~/Projects/nginx/ngx_mem_pool$ ./bin/demo1 1
use_pool
CPU Time Used: 5.100224 seconds #（使用内存池，自旋锁同步线程）
```


[@George-Hotz](https://github.com/George-Hotz/Nginx_Memory_Pool)
