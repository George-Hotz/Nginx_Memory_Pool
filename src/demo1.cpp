
/*
    Author: George-Hotz(雪花)
    Github：https://github.com/George-Hotz/Nginx_Memory_Pool
*/

#include <time.h>
#include <malloc.h>
#include "ngx_core.h"
#include "ngx_palloc.h"

#define S_BLOCK_SIZE 8                  //小内存

#define INNER_CYCLE_NUM 1024            //内循环次数
#define OUTER_CYCLE_NUM (1024 * 16)     //外循环次数

int main(int argc, char **argv){
    int i = 0, k = 0;
    int use_pool = 0;

    if(argc > 2){
        printf("numbers of paramters is 0 or 1\n");
        return 0;
    }

    if(argc == 1){
        use_pool = 0;
        printf("use_malloc\n");
    }else{
        use_pool = 1;
        printf("use_pool\n");
    }

    clock_t start, end;
    double cpu_time_used;

    start = clock();  // 记录开始时间

    if(use_pool){
        char *ptr = nullptr;
        for(k=0; k<OUTER_CYCLE_NUM; k++){
            for(int i=0; i<INNER_CYCLE_NUM; i++){
                ptr = (char*)Ngx_Mem_Pool::get_instance().ngx_palloc(S_BLOCK_SIZE);
                if(!ptr){
                    std::cerr << "ngx_palloc failed." << std::endl;
                }else{
                    *ptr = '\0';
                    *(ptr + S_BLOCK_SIZE - 1) = '\0';
                }
            }
            Ngx_Mem_Pool::get_instance().ngx_reset_pool();
        }
    }else{
        char *ptr = nullptr;
        for(k=0; k<OUTER_CYCLE_NUM; k++){
            for(int i=0; i<INNER_CYCLE_NUM; i++){
                ptr = (char*)malloc(S_BLOCK_SIZE);
                if(!ptr){
                    std::cerr << "malloc failed." << std::endl;
                }else{
                    *ptr = '\0';
                    *(ptr + S_BLOCK_SIZE - 1) = '\0';
                }
                free(ptr);
            }
        }
    }
    
    end = clock();   // 记录结束时间
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; // 计算CPU时间
    printf("CPU Time Used: %f seconds\n", cpu_time_used);

    return 0;
}