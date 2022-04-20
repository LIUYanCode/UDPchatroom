#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define ERRLOG(errmsg) do{\
                            perror(errmsg);\
                            printf("%s - %s - %d\n", __FILE__, __func__, __LINE__);\
                            exit(1);\
                        }while(0)
typedef struct{
    int code;
    char name[32];
    char text[32];
}MSG;

typedef struct node{
    struct sockaddr_in addr;
    struct node *next;
}linklist;

linklist *linklistCreate();
void udpChatLogin(int sockfd, MSG msg, linklist *h, struct sockaddr_in clientaddr);
void udpChatBroadcast(int sockfd, MSG msg, linklist *h, struct sockaddr_in clientaddr);
void udpChatQuit(int sockfd, MSG msg, linklist *h, struct sockaddr_in clientaddr);
int main(int argc, char const *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen = sizeof(serveraddr);

    //创建套接字
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        ERRLOG("socket error");
    }

    //填充服务器网络信息结构体
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(atoi(argv[2]));

    //将套接字与服务器网络信息结构体绑定
    if(bind(sockfd, (struct sockaddr *)&serveraddr, addrlen) == -1)
    {
        ERRLOG("bind error");
    }

    //创建子进程实现一边接收数据并处理一边发送系统信息
    pid_t pid;
    if((pid = fork()) == -1)
    {
        ERRLOG("fork error");
    }
    else if(pid > 0) //父进程负责发送系统信息
    {
        //将父进程当做客户端直接给子进程发送群聊信息，
        //子进程接收到之后将其当做群聊数据发送给所有用户即可
        MSG msg;
        //设置操作码
        msg.code = 2;
        //设置用户名
        strcpy(msg.name, "server");
        while(1)
        {
            //数据要发送的系统信息
            fgets(msg.text, 32, stdin);
            msg.text[strlen(msg.text) - 1] = '\0';

            if(sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr, addrlen) == -1)
            {
                ERRLOG("sendto error");
            }
        }
    }
    else //子进程接收数据并处理
    {
        MSG msg;
        linklist *h = linklistCreate();
        while(1)
        {
            if(recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&clientaddr, &addrlen) == -1)
            {
                ERRLOG("recvfrom error");
            }

            printf("%d:%s:%s\n", msg.code, msg.name, msg.text);

            //根据接收数据中的操作码做出相应的处理
            switch (msg.code)
            {
            case 1: //登录操作
                udpChatLogin(sockfd, msg, h, clientaddr);
                break;
            case 2: //群聊操作
                udpChatBroadcast(sockfd, msg, h, clientaddr);
                break;
            case 3: //退出操作
                udpChatQuit(sockfd, msg, h, clientaddr);
                break;
            }
        }
    }
}

linklist *linklistCreate()
{
    linklist *h = (linklist *)malloc(sizeof(linklist));
    h->next = NULL;

    return h;
}

void udpChatLogin(int sockfd, MSG msg, linklist *h, struct sockaddr_in clientaddr)
{
    char buf[128] = {0};
    //将新用户上线信息组包发送给其他在线用户
    //组包
    sprintf(buf, "--------------%s 上线了---------------", msg.name);
    //将数据发送给所有在线的用户
    //遍历链表并给数据域发送数据
    linklist *p = h;
    while(p->next != NULL)
    {
        p = p->next;
        if(sendto(sockfd, buf, 128, 0, (struct sockaddr *)&p->addr, sizeof(p->addr)) == -1)
        {
            ERRLOG("sendto error");
        }
    }

    //将新用户的信息保存在链表中
    linklist *temp = (linklist *)malloc(sizeof(linklist));
    temp->addr = clientaddr;
    temp->next = NULL;

    temp->next = h->next;
    h->next = temp;

    return ;
}

void udpChatBroadcast(int sockfd, MSG msg, linklist *h, struct sockaddr_in clientaddr)
{
    //将用户说的话组包发送给其他在线用户
    char buf[128] = {0};
    sprintf(buf, "%s: %s", msg.name, msg.text);

    //遍历链表并发送
    linklist *p = h;
    while(p->next != NULL)
    {
        p = p->next;

        //发送者自己不接收数据
        //memcmp：比较两块内存区域的内容是否一样
        if(memcmp(&p->addr, &clientaddr, sizeof(clientaddr)) != 0)
        {
            if(sendto(sockfd, buf, 128, 0, (struct sockaddr *)&p->addr, sizeof(p->addr)) == -1)
            {
                ERRLOG("sendto error");
            }
        }
    }
}

void udpChatQuit(int sockfd, MSG msg, linklist *h, struct sockaddr_in clientaddr)
{
    //组包并发送数据
    char buf[128] = {0};
    sprintf(buf, "-------------%s 下线了-------------", msg.name);

    //遍历链表并发送
    linklist *p = h;
    linklist *temp;
    while(p->next != NULL)
    {
        //将要退出用户的信息从链表中删除
        if(memcmp(&p->next->addr, &clientaddr, sizeof(clientaddr)) == 0)
        {
            temp = p->next;
            p->next = temp->next;

            free(temp);
            temp = NULL;
        }
        else 
        {
            if(sendto(sockfd, buf, 128, 0, (struct sockaddr *)&p->next->addr, sizeof(p->addr)) == -1)
            {
                ERRLOG("sendto error");
            }

            p = p->next;
        }
    }
}