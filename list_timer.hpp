#ifndef LST_TIMER_H
#define LIST_TIMER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

class util_timer;

struct client_data
{
    struct sockaddr_in address_;
    int sockfd_;
    char buf[1024];
    util_timer* timer_;    
};

class util_timer
{
public:
    explicit util_timer(int expire = 0, int interval = 0.0)
            : expire_(expire)
            , interval_(interval)
            , repeated_(interval > 0.0)
            , next_(nullptr)
            , prev_(nullptr)
    {}

    time_t expire_;
    double interval_;
    bool repeated_;
    void (*cb_func)(client_data*);
    client_data* user_data;

    util_timer *next_;
    util_timer *prev_;
};

class sort_timer_list
{
public:
    explicit sort_timer_list()
                : head_(new util_timer())
                , tail_(new util_timer())
    {
        head_->next_ = tail_;
        tail_->prev_ = head_;
    }
    ~sort_timer_list()
    {
        util_timer *tmp = nullptr;
        while (head_) {
            tmp = head_->next_;
            delete head_;
            head_ = tmp;
        }
    }
    sort_timer_list(const sort_timer_list& stimer) = delete;
    sort_timer_list& operator= (const sort_timer_list& stimer) = delete;

    void add_timer(util_timer* timer);

    //这个定时器被给了新的定时时间，需要重新调整位置
    void adjust_timer(util_timer *timer);

    void del_timer(util_timer* timer);

    void tick();
private:
    util_timer *head_;
    util_timer *tail_;
};

#endif 