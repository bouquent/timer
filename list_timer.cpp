#include "list_timer.hpp"

void sort_timer_list::add_timer(util_timer *timer)
{
    if (timer == nullptr) {
        return ;
    }
    if (head_->next_ == tail_) {
        //只有头尾节点
        timer->next_ = tail_;
        timer->prev_ = head_;
        head_->next_ = timer;
        tail_->prev_ = timer;
        return ;
    }

    util_timer* tmp = head_->next_;
    while (tmp != tail_ && timer->expire_ > tmp->expire_) {
        tmp = tmp->next_;
    }
    
    timer->next_ = tmp;
    timer->prev_ = tmp->prev_;
    tmp->prev_->next_ = timer;
    tmp->prev_ = timer;
}

void sort_timer_list::del_timer(util_timer* timer)
{
    if (nullptr == timer) {
        return ;
    }

    timer->next_->prev_ = timer->prev_;
    timer->prev_->next_ = timer->next_;
    delete timer;
}

void sort_timer_list::adjust_timer(util_timer *timer)
{
    if (nullptr == timer) {
        return ;
    }
    
    time_t newExpire = timer->expire_; //获取新的超时事件
    int interval = timer->interval_;
    del_timer(timer);

    timer = new util_timer(newExpire, interval);
    add_timer(timer);
}

void sort_timer_list::tick()
{
    time_t now = time(NULL);
    
    if (head_->next_ == tail_) {
        return ;
    }
    while(head_->next_ != tail_) {
        util_timer* tmp = head_->next_;
        if (now >= tmp->expire_) {
            tmp->cb_func(tmp->user_data);

            tmp->next_->prev_ = tmp->prev_;
            tmp->prev_->next_ = tmp->next_;
            if (!tmp->repeated_) {
                delete tmp;
            } else {
                //如果这个定时器重复，则需要重新挂到链表上去
                tmp->expire_ = now + tmp->interval_;
                adjust_timer(tmp);
            }
        } else {
            break;
        }
    }
}

