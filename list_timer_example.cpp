#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "list_timer.hpp"
#define TIMECOUNT 2
sort_timer_list timers;
client_data cli_data[1000] = {0};
int epollfd = 0;
int timerfd = 0;

void cb_client(client_data *data)
{
    char buf[100] = {0};
    sprintf(buf, "this is my heart bag!\n");
    write(data->sockfd_, buf, sizeof(buf));
    printf("send heart bag to %d\n", data->sockfd_);
}


void addEvent(int epollfd, int sockfd)
{
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;

    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    assert(ret == 0);
}

void setTimer()
{
    itimerspec newtime;
    itimerspec oldtime;
    memset(&newtime, 0, sizeof(newtime));
    memset(&oldtime, 0, sizeof(oldtime));

    newtime.it_value.tv_sec = TIMECOUNT;
    int ret = timerfd_settime(timerfd, 0, &newtime, &oldtime);
    assert(ret != -1);
}

void setNonBlock(int sockfd)
{
    int flag = fcntl(sockfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flag);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("please vaild address and port!\n");\
        exit(1);
    }
    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(atoi(argv[2]));
    ser_addr.sin_addr.s_addr = inet_addr(argv[1]);

    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    assert(listenfd != -1);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    int ret = bind(listenfd, (struct sockaddr*)& ser_addr, sizeof(ser_addr));
    assert(ret == 0);


    epollfd = epoll_create(1000);
    struct epoll_event events[1000];
    addEvent(epollfd, listenfd);
    
    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    addEvent(epollfd, timerfd);
    setTimer();

    listen(listenfd, 13);

    while (1) {
        int nready = epoll_wait(epollfd, events, 1000, 0);
        assert(nready >= 0);
        for (int i = 0; i < nready; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in cli_addr;
                socklen_t len = sizeof(cli_addr);
                int connfd = accept(listenfd, (struct sockaddr*) &cli_addr, &len);
                assert(connfd != -1);
                setNonBlock(connfd);

                char buf[30] = {0};
                printf("connect a new clien, address is :%s, port is :%u\n", 
                    inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, buf, sizeof(buf)),
                    ntohs(cli_addr.sin_port));

                addEvent(epollfd, connfd);
                
                time_t now = time(nullptr);
                util_timer *timer = new util_timer(now + 3 * TIMECOUNT, 2.0);
                cli_data[connfd].sockfd_ = connfd;
                cli_data[connfd].address_ = cli_addr;
                cli_data[connfd].timer_ = timer;
                timer->cb_func = cb_client;
                timer->user_data = &cli_data[connfd];
                timers.add_timer(timer);  
            } else if (sockfd == timerfd){
                uint64_t howmany = 0;
                int ret = read(timerfd, &howmany, sizeof(howmany));
                if (ret != sizeof(howmany)) {
                    printf("read timerfd wong!\n");
                }
                printf("time get to sort_timer_list.tick()!\n");
                timers.tick();
                setTimer();
            } else {
                char buf[100] = {0};
                int ret = read(sockfd, buf, sizeof(buf));
                if (ret > 0) {
                    write(sockfd, buf, ret);
                } else if (ret == 0) {
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, nullptr); //从epoll红黑树上摘下
                    
                    client_data data = cli_data[sockfd];
                    timers.del_timer(data.timer_);       //删除对应的定时器
                    close(sockfd);      //删除socket文件描述符
                }
            }
        }
    }

    close(listenfd);
    close(epollfd);
    return 0;
}
