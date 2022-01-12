#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <strings.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define TIMECOUNT 2

#include "heap_timer.hpp"

timer_heap timers;
struct client_data cli_data[1024];

void client_cb(client_data* client_d)
{
    char buf[] = "this is my heart bag!\n";
    write(client_d->sockfd_, buf, strlen(buf) + 1);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("please input vaild address and port!\n");
        exit(1);
    }
    struct sockaddr_in ser_addr;
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(atoi(argv[2]));
    ser_addr.sin_addr.s_addr = inet_addr(argv[1]);


    int listenfd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    assert(listenfd != -1);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    int ret = bind(listenfd, (struct sockaddr*) &ser_addr, sizeof(ser_addr));
    assert(ret == 0);

    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    assert(timerfd != -1);

    struct pollfd pfds[1024];
    for (int i = 0; i < 1024; ++i) pfds[i].fd = -1;
    pfds[0].fd = listenfd;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = timerfd;
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;
    int maxi = 1, i;

    
    listen(listenfd, 13);
    while (1) {
        int nready = poll(pfds, 1024, -1);
        assert(nready >= 0);
        if (nready == 0) {
            continue;
        }
        if (pfds[0].revents == POLLIN) {
            struct sockaddr_in cli_addr;
            socklen_t len = sizeof(cli_addr);
            int connfd = accept(listenfd, (struct sockaddr*) &cli_addr, &len);
            assert(connfd != -1);
            char buf[30] = {0};
            printf("connect client address : %s, port : %u\n", 
                inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, buf, sizeof(buf)),
                ntohs(cli_addr.sin_port));

            for (i = 1; i <= 1024; ++i) {
                if (pfds[i].fd == -1) break;
            }
            if (i == 1024) {
                printf("the sockfd is too many!\n");
                continue;
            }
            if (i > maxi) maxi = i;
            pfds[i].fd = connfd;
            pfds[i].events = POLLIN;
            pfds[i].revents = 0;

            time_t now = time(nullptr) + 2 * TIMECOUNT;
            heap_timer* ht = new heap_timer(now, 2);
            cli_data[i].sockfd_ = connfd;
            cli_data[i].timer_ =  ht;
            ht->cb_func = client_cb;
            ht->cli_data_ = &cli_data[i];     
            timers.add_timer(ht);

            if (--nready == 0) {
                continue;
            }
        }
        if (pfds[1].events == POLLIN) {
            //定时时间到，处理定时任务
            uint64_t howmany;
            int ret = read(pfds[1].fd, &howmany, sizeof(howmany));
            timers.tick();
            
            //重新定时
            struct itimerspec newtime;
            struct itimerspec oldtime;
            memset(&newtime, 0, sizeof(newtime));
            memset(&oldtime, 0, sizeof(oldtime));
            newtime.it_value.tv_sec = TIMECOUNT;
            timerfd_settime(pfds[1].fd, 0, &newtime, &oldtime);
        }

        for (int i = 2; i <= maxi; ++i) {
            if (pfds[i].revents == POLLIN) {
                char buf[100] = {0};
                int ret = read(pfds[i].fd, buf, sizeof(buf));
                if (ret == 0) {
                    close(pfds[i].fd);
                    pfds[i].fd = -1;
                    pfds[i].events = 0;
                    
                    timers.del_timer(cli_data[i].timer_);
                } else if (ret > 0) {
                    //写回
                    write(pfds[i].fd, buf, ret);
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }
    }

    close(listenfd);
    return 0;
}