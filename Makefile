


#                                                              
#   Author: George-Hotz(雪花)                                  
#   Github：https://github.com/George-Hotz/Nginx_Memory_Pool   
#                                                              



# 定义编译器
CC=g++

# 定义编译选项
CFLAGS=-Wall -Wextra

# 定义链接选项
LDFLAGS=

# 定义目标文件
TARGET1=./bin/demo1 
TARGET2=./bin/demo2

# 定义源文件
SOURCE1 = ./src/demo1.cpp ./src/ngx_palloc.cpp
SOURCE2 = ./src/demo2.cpp ./src/ngx_palloc.cpp

# 默认目标
all: $(TARGET1) $(TARGET2)

# 目标依赖于源文件
$(TARGET1): $(SOURCE1)
	$(CC) -O3 $(CFLAGS) $(LDFLAGS) -o $@ $^

$(TARGET2): $(SOURCE2)
	$(CC) -O3 $(CFLAGS) $(LDFLAGS) -o $@ $^

# 编译每个源文件为对象文件
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译生成的文件
clean:
	rm -f $(TARGET1) $(TARGET2) *.o

# 伪目标，用于打印变量
.PHONY: all clean