#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

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
int main(int argc, char const *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serveraddr;
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

    MSG msg;
    
    //执行登录操作
    //设置操作码标识要登录
    msg.code = 1;
    //设置用户名
    printf("请输入用户名：");
    fgets(msg.name, 32, stdin);
    msg.name[strlen(msg.name) - 1] = '\0';
    //发送数据
    if(sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr, addrlen) == -1)
    {
        ERRLOG("sendto error");
    }

    //创建子进程实现一边发送数据一边接收数据
    pid_t pid;
    if((pid = fork()) == -1)
    {
        ERRLOG("fork error");
    }
    else if(pid > 0) //父进程负责发送数据
    {
        while(1)
        {
            fgets(msg.text, 32, stdin);
            msg.text[strlen(msg.text) - 1] = '\0';
            
            //退出操作
            if(strcmp(msg.text, "quit") == 0)
            {
                //设置操作码为3标识为退出操作
                msg.code = 3;

                //将组好的数据发送给服务器
                if(sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr, addrlen) == -1)
                {
                    ERRLOG("sendto error");
                }
                //先让子进程结束
                kill(pid, SIGKILL);
                //父进程也结束
                exit(0);
            }
            //群聊操作
            else 
            {
                //设置操作码为2标识为群聊操作
                msg.code = 2;
                //将组好的数据发送给服务器
                if(sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serveraddr, addrlen) == -1)
                {
                    ERRLOG("sendto error");
                }
            }  
        }
    }
    else //子进程负责接收数据 
    {
        char buf[128] = {0};
        while(1)
        {
            if(recvfrom(sockfd, buf, 128, 0, NULL, NULL) == -1)
            {
                ERRLOG("recvfrom error");
            }

            printf("%s\n", buf);
        }
    }
}