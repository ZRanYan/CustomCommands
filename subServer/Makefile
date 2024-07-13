# Makefile

CC = gcc
CFLAGS = -Wall -I.   # 编译选项，包含当前目录（用于找到cJSON.h头文件）
LDFLAGS = -lcjson  -pthread -lm # 链接选项，链接cJSON库

TARGET = subServer    # 目标程序名

SRCS = subServer.c  debug.c      # 源文件列表
OBJS = $(SRCS:.c=.o)  # 生成的目标文件列表

all: $(TARGET)

$(TARGET): $(OBJS)
	./generate_version_header.sh
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
