
/*
    Github：https://github.com/George-Hotz/Nginx_Memory_Pool
 */

#include <time.h>
#include <malloc.h>
#include "ngx_core.h"
#include "ngx_alloc.h"
#include "ngx_palloc.h"

#define BLOCK_SIZE 8                    //每次申请的内存大小
#define MEM_POOL_SIZE (1024 * 4)

#define INNER_CYCLE_NUM 1024             //内循环次数
#define OUTER_CYCLE_NUM (1024 * 1024)     //外循环次数

int main(int argc, char **argv){
    int i = 0, k = 0;
    int use_pool = 0;

    if(argc > 2){
        printf("numbers of paramters is 1 or 2\n");
        return 0;
    }

    if(argc == 1){
        use_pool = 0;
        printf("use_malloc\n");
    }else{
        use_pool = 1;
        printf("use_pool\n");
    }

    ngx_pagesize = getpagesize();

    clock_t start, end;
    double cpu_time_used;

    start = clock();  // 记录开始时间

    if(use_pool){
        char *ptr = NULL;
        for(k=0; k<OUTER_CYCLE_NUM; k++){
            ngx_pool_t *pool = ngx_create_pool(MEM_POOL_SIZE);
            for(i=0; i<INNER_CYCLE_NUM; i++){
                ptr = ngx_palloc(pool, BLOCK_SIZE);
                if(!ptr){
                    printf(stderr, "ngx_palloc failed. \n");
                }else{
                    *ptr = '\0';
                    *(ptr + BLOCK_SIZE - 1) = '\0';
                }
            }
            ngx_destroy_pool(pool);
        }
    }else{
        char *ptr[INNER_CYCLE_NUM];
        for(k=0; k<OUTER_CYCLE_NUM; k++){
            for(i=0; i<INNER_CYCLE_NUM; i++){
                ptr[i] = malloc(BLOCK_SIZE);
                if(!ptr){
                    printf(stderr, "malloc failed. \n");
                }else{
                    *ptr = '\0';
                    *(ptr + BLOCK_SIZE - 1) = '\0';
                }
            }
            for(i=0; i<INNER_CYCLE_NUM; i++){
                if(ptr[i]){
                    free(ptr[i]);
                }
            }
        }
    }
    
    end = clock();   // 记录结束时间
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; // 计算CPU时间
    printf("CPU Time Used: %f seconds\n", cpu_time_used);

    return 0;
}