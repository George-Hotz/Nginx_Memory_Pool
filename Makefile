# Github：
# https://github.com/George-Hotz/Nginx_Memory_Pool

# 定义编译器
CC=g++

# 定义编译选项
CFLAGS=-Wall -Wextra

# 定义链接选项
LDFLAGS=

# 定义目标文件
TARGET=./bin/ngx_mem_pool

# 定义源文件
SOURCE = ./src/main.cpp ./src/ngx_palloc.cpp

# 默认目标
all: $(TARGET)

# 目标依赖于源文件
$(TARGET): $(SOURCE)
	$(CC) -O3 $(CFLAGS) $(LDFLAGS) -o $@ $^

# 编译每个源文件为对象文件
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译生成的文件
clean:
	rm -f $(TARGET) *.o

# 伪目标，用于打印变量
.PHONY: all clean