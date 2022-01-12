#include "heap_timer.hpp"
#include <time.h>
#include <algorithm>


void timer_heap::pop()
{
    if (empty()) return ;

    delete timer_array[0];
    timer_array[0] = timer_array[--cur_size_];   //因为数组下标从0开始，计数从1开始，所以是--cur_size_
    heapjust(0);
}

/*
                    0
            1             2
        3       4      5      6
*/
void timer_heap::add_timer(heap_timer* timer)
{
    if (!timer) return ;
    if (capacity_ == cur_size_) {
        resize();
    }

    int index;
    int newtimer = cur_size_++;

    for (index = (newtimer - 1) / 2; newtimer > 0; index = (index - 1) / 2) {
        if (timer_array[index]->expire_ > timer->expire_) {
            timer_array[newtimer] =  timer_array[index];
            newtimer = index;
        } else {
            break;
        }
    }
    timer_array[newtimer] = timer;
}

void timer_heap::del_timer(heap_timer* timer)
{
    if (timer == nullptr) {
        return ;
    }

    //将timer的cb滞空，那么这个定时器将一直往上走，直到成为头节点被删除。
    timer->cb_func = nullptr;
    timer->repeat_ = false;
}
    
void timer_heap::tick()
{
    time_t now = time(nullptr);
    while (!empty()) {
        heap_timer* tmp = timer_array[0];
        if (now < tmp->expire_) {
            break;
        }
        
        if (tmp->cb_func) {
            tmp->cb_func(tmp->cli_data_);
        }

        if (tmp->repeat_) {
            time_t newtime = now + tmp->interval_;
            tmp->expire_ = newtime;
            heapjust(0);
        } else {
            this->pop();
        }
    }
}


void timer_heap::heapjust(int index)
{
    heap_timer* tmp = timer_array[index];
    int i;
    for (i = index * 2 + 1; i < cur_size_; i = 2 * i + 1) {
        if (i + 1 < cur_size_ && timer_array[i]->expire_ > timer_array[i + 1]->expire_) {
            //选出左右子树中更小的节点
            ++i;
        }
        if (tmp->expire_ <= timer_array[i]->expire_) {
            break;
        }
        timer_array[index] = timer_array[i];
        index = i;
    }
    timer_array[index] = tmp;
}

void timer_heap::resize()
{
    //两倍扩容
    int newcapacity = capacity_ * 2;
    heap_timer** tmptimer_array = new heap_timer*[newcapacity];
    for (int i = 0; i < newcapacity; ++i) {
        tmptimer_array[i] = nullptr;
    }

    for (int i = 0; i < capacity_; ++i) {
        tmptimer_array[i] = timer_array[i];
    }
    std::swap(tmptimer_array, timer_array);
    delete []tmptimer_array;
}
