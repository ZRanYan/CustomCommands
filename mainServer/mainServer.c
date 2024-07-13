#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cjson/cJSON.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#include <poll.h>
#include <string.h>

#define BUFF_SIZE 2048
#define SERVER_PORT 9995
#define LOCAL_HOST "127.0.0.1"
#define LENGTH_OF_LISTEN_QUEUE 10
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define MAX_CLIENTS 10


typedef int (*FUNC_PROCESSING)(char *);

// 客户端连接信息结构体
struct ClientInfo
{
    int sockfd;
    struct sockaddr_in addr;
};

typedef struct
{
    unsigned short int mark;        // 固定标识符
    unsigned short int data_length; // 数据长度
    unsigned short int id;//程序id编号
} PacketHeader;

typedef enum
{
    IP = 0,
    MASK = 1,
    GATE_WAY = 2,
    SET_FPG_OPT = 3,
    GET_VERSION = 4,
} Program_ID_Enum;

typedef struct
{
    Program_ID_Enum id;
    FUNC_PROCESSING funcPtr;
} ProcessFunctionStructures;

int getOutPutVersion(char *data)
{
    int idNum;
    char *string;
    cJSON *root = cJSON_Parse(data);
    if (root == NULL)
    {
        return -1;
    }
    cJSON *id = cJSON_GetObjectItem(root, "id");
    if(NULL!= id)
    {
        idNum = id->valueint;
    }
    cJSON *notes = cJSON_GetObjectItem(root, "notes");
    if(NULL != notes)
    {
        string = notes->valuestring;
    }
    printf("id:%d  string:%s\n", idNum, string);
    cJSON_Delete(root);
    return 0;
}

int setFpgaData(char *data)
{

    return 0;
}

ProcessFunctionStructures mFunctionArray[]=
{
    {GET_VERSION, &getOutPutVersion},
    {SET_FPG_OPT, &setFpgaData},
};


// 处理客户端连接的线程函数
void *handle_client(void *arg)
{
    struct ClientInfo *client_info = (struct ClientInfo *)arg;
    int client_sockfd = client_info->sockfd;
    // struct sockaddr_in client_addr = client_info->addr;
    char buffer[BUFF_SIZE];
    char replyContent[10];
    PacketHeader headInfo;
    int ret = 0;
    // char client_ip[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    // printf("Handling client from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
    memset(buffer, 0, sizeof(buffer));
    int recvHeadLength = recv(client_sockfd, &headInfo, sizeof(headInfo), 0);
    if (sizeof(headInfo) == recvHeadLength)
    {
        if (headInfo.data_length == recv(client_sockfd, buffer, headInfo.data_length, 0))
        {
            // printf("recvBuff:%s\n", buffer);
            for(int i=0;i<sizeof(mFunctionArray)/sizeof(mFunctionArray[0]);i++)
            {
                if(headInfo.id == mFunctionArray[i].id)
                {
                    ret = mFunctionArray[i].funcPtr(buffer);
                    break;
                }
            }
            if(0 == ret)
            {
                strncpy(replyContent, "ok", 10);
            }
            else
            {
                strncpy(replyContent, "failed", 10);
            }
            if ((send(client_sockfd, replyContent, sizeof(replyContent), MSG_DONTWAIT)) == -1)
            {
                printf("error \n");
            }
        }
    }
    // 关闭客户端连接
    close(client_sockfd);
    free(client_info);
    pthread_exit(NULL);
}

int main()
{
    int listen_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    struct pollfd fds[MAX_CLIENTS + 1]; // 加上一个用来监听监听socket
    int num_fds = 1;                    // 初始化为1，因为初始只有监听socket
    int i, ret;

    // 创建监听socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1)
    {
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址为本地回环地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 只接受本地连接
    server_addr.sin_port = htons(SERVER_PORT);

    // 绑定地址和端口
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed \n");
        exit(EXIT_FAILURE);
    }

    // 监听连接，设置 backlog 参数为 10
    if (listen(listen_sockfd, 10) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // 初始化pollfd结构体数组
    fds[0].fd = listen_sockfd;
    fds[0].events = POLLIN;
    printf("Waiting for incoming connections...\n");

    // 主循环
    while (1)
    {
        // 使用poll等待事件发生
        ret = poll(fds, num_fds, -1);
        if (ret == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        // 检查每个文件描述符
        for (i = 0; i < num_fds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == listen_sockfd)
                {
                    // 新的连接请求
                    client_addr_len = sizeof(client_addr);
                    client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (client_sockfd == -1)
                    {
                        perror("accept");
                        continue;
                    }
                    // 打印连接信息
                    // char client_ip[INET_ADDRSTRLEN];
                    // inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                    // printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
                    // 将新的客户端连接添加到pollfd数组中
                    if (num_fds < MAX_CLIENTS + 1)
                    { // 防止超出数组界限
                        fds[num_fds].fd = client_sockfd;
                        fds[num_fds].events = POLLIN;
                        num_fds++;
                    }
                    else
                    {
                        fprintf(stderr, "ERROR Too many clients. Rejecting new connection.\n");
                        close(client_sockfd);
                    }

                    // 创建新线程处理客户端接收数据
                    struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
                    client_info->sockfd = client_sockfd;
                    client_info->addr = client_addr;

                    pthread_t tid;
                    if (pthread_create(&tid, NULL, handle_client, (void *)client_info) != 0)
                    {
                        perror("pthread_create error");
                        close(client_sockfd);
                        free(client_info);
                    }
                    else
                    {
                        // 线程创建成功，将线程设置为分离状态
                        pthread_detach(tid);
                    }
                }
                else
                {
                    // 已连接的客户端有数据可读，暂不处理，留给线程处理
                }
            }
            else if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                // printf("Client disconnected from fd %d\n", fds[i].fd);
                // 将当前文件描述符从pollfd数组中移除
                if (i < num_fds - 1)
                {
                    fds[i] = fds[num_fds - 1];
                }
                num_fds--;
                i--; // 因为移除了一个元素，所以需要重新检查当前位置
            }
        }
    }
    // 关闭监听socket
    close(listen_sockfd);

    return 0;
}