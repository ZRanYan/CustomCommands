# Makefile

CC = gcc
CFLAGS = -Wall -I.   # 编译选项，包含当前目录（用于找到cJSON.h头文件）
LDFLAGS = -lcjson  -pthread  # 链接选项，链接cJSON库

TARGET = info    # 目标程序名

SRCS = mainServer.c         # 源文件列表
OBJS = $(SRCS:.c=.o)  # 生成的目标文件列表

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)
	./$(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
