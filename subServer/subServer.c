#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjson/cJSON.h"
#include "debug.h"
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE           2048
#define SERVER_PORT         9995
#define MARK_CHAR           0x0A0B
#define RECV_LENGTH         10  //回复固定的信息长度
#define LOCAL_HOST          "127.0.0.1"
#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct {
    unsigned short int mark;//固定标识符
    unsigned short int data_length;  // 数据长度
    unsigned short int id;//程序id编号
}PacketHeader;

typedef enum
{
    IP = 0,
    MASK = 1,
    GATE_WAY = 2,
    SET_FPG_OPT = 3,
    GET_VERSION = 4,
} Program_ID_Enum;

typedef unsigned int (*FuncPtr)(Program_ID_Enum, int, char *[], char *); // 函数处理指针

typedef struct
{
    Program_ID_Enum id;
    char programName[20]; // 程序名字
    FuncPtr funcPtr;
} ProcessFunctionStructures;

unsigned int setIpConfigDevice(Program_ID_Enum id, int argc, char *ip[], char *outbuf)
{
    printf("id:%d %d ip:%s\n", id, argc, ip[1]);
    exit(EXIT_SUCCESS);
}

unsigned int setMaskConfigDevice(Program_ID_Enum id, int argc, char *mask[], char *outbuf)
{
    printf("id:%d %d mask:%s\n", id, argc, mask[1]);
    exit(EXIT_SUCCESS);
}

unsigned int setFpgaCOnfig(Program_ID_Enum id, int argc, char *mask[], char *outbuf)
{
    return 0;
}

unsigned int getOutputVersion(Program_ID_Enum id, int argc, char *mask[], char *outbuf)
{
    int retLength = 0;
    cJSON *root = cJSON_CreateObject();
    printf("sgdss\n");
    if(NULL == root)
    {
        DEBUG_LOG(APP, ERROR, "getOutputVersion create root failed");
        return 0;
    }
    cJSON_AddNumberToObject(root, "id", (int)id);
    cJSON_AddStringToObject(root, "notes", "Retrieve the version information of the program");
    char *json_str = cJSON_PrintUnformatted(root);
    retLength = min(strlen(json_str), BUFF_SIZE);
    memcpy(outbuf, json_str, retLength);
    cJSON_Delete(root);
    free(json_str);
    return retLength;
}


// 带有超时机制的阻塞接收函数实现
int recv_with_timeout(int socket, void *buffer, size_t length, int timeout_sec) {
    fd_set fds;
    struct timeval tv;
    int retval;

    // 清空文件描述符集合
    FD_ZERO(&fds);
    // 将套接字加入文件描述符集合
    FD_SET(socket, &fds);

    // 设置超时时间
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    // 等待套接字可读
    retval = select(socket + 1, &fds, NULL, NULL, &tv);
    if (retval == -1) {
        perror("select");
        return -1;
    } else if (retval == 0) {
        // 超时
        printf("Timeout occurred!\n");
        return 0;
    } else {
        // 套接字可读，进行接收操作
        return recv(socket, buffer, length, 0);
    }
}

ProcessFunctionStructures mFuncData[] =
{
    {IP,    "setIp",                &setIpConfigDevice},
    {MASK,  "setMask",              &setMaskConfigDevice},
    {SET_FPG_OPT, "setFpga",        &setFpgaCOnfig},
    {GET_VERSION, "outPutVersion",  &getOutputVersion},
};

int main(int argc, char *argv[])
{
    
    char sendBuffer[BUFF_SIZE];
    unsigned int sendDataLength = 0;
    char recvBuff[RECV_LENGTH];
    int i = 0;
    PacketHeader packHead = 
    {
        .mark = MARK_CHAR,
        .data_length = 0,
    };
    memset(sendBuffer, 0, BUFF_SIZE);
    memset(recvBuff, 0, RECV_LENGTH);

    memcpy(sendBuffer, &packHead, sizeof(packHead));

    for (i = 0; i < sizeof(mFuncData) / sizeof(mFuncData[0]); i++)
    {
        if (0 == strcmp(argv[0], mFuncData[i].programName))
        {
            sendDataLength = mFuncData[i].funcPtr(mFuncData[i].id, argc, argv, sendBuffer + sizeof(packHead));
            break;
        }
    }
    // printf("128-buffer:%s\n", sendBuffer + sizeof(packHead));
    packHead.data_length = sendDataLength;
    packHead.id = mFuncData[i].id;
    memcpy(sendBuffer, &packHead, sizeof(packHead));
    sendDataLength += sizeof(packHead);
    struct sockaddr_in client_addr;                     //客户端IP配置
    bzero(&client_addr,sizeof(client_addr));
    
    client_addr.sin_family = AF_INET;                   
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);    //设置主机IP的地址
    client_addr.sin_port = htons(0);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);  
    if (client_socket < 0)  
    {  
        printf("Create Socket Failed!\n");  
        exit(1);  
    } 
    if(bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
    {
        printf("Client Bind Port failed!\n");
        exit(1);
    }
    struct sockaddr_in server_addr;                     //服务器端IP的配置
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family=AF_INET;

    if(inet_aton(LOCAL_HOST,&server_addr.sin_addr) == 0){
        printf("Server IP Address Error!\n");
        exit(1);
    }
    server_addr.sin_port=htons(SERVER_PORT);
    socklen_t server_addr_length=sizeof(server_addr);
    if(connect(client_socket,(struct sockaddr*)&server_addr,server_addr_length)<0)      //连接服务器
    {
        printf("Can not Connect to %s\n", LOCAL_HOST);
        exit(1);
    }
    //发送配置报文
    if((send(client_socket, sendBuffer, sendDataLength, 0))==-1)
    {
        printf("send error!\n");
    }
    
    int recvLength = recv_with_timeout(client_socket, recvBuff, RECV_LENGTH, 1);

    if( 0 != recvLength)
    {
        printf("recvLength  %d %s\n", recvLength, recvBuff);
    }
    close(client_socket);

    return 0;
}
