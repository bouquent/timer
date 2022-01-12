#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#define HEAP_MAX_SIZE  64
class heap_timer;

struct client_data
{
    int sockfd_;
    heap_timer* timer_;
};

class heap_timer
{
public:
    heap_timer(int expire = 0, double interval = 0.0)
            : expire_(expire)
            , interval_(interval)
            , repeat_(interval > 0.0)
    {}
    int expire_;
    double interval_;
    int repeat_;
    client_data* cli_data_;

    void (*cb_func)(client_data*);
};

class timer_heap
{
public:
    timer_heap(int capacity = HEAP_MAX_SIZE)
            : timer_array(nullptr)
            , capacity_(capacity)
            , cur_size_(0)
    {
        timer_array = new heap_timer*[capacity];
        for (int i = 0; i < capacity; ++i) {
            timer_array[i] = nullptr;
        }
    }
    ~timer_heap()
    {
        for (int i = 0; i < cur_size_; ++i) {
            if (timer_array[i] != nullptr) {
                delete timer_array[i];
                timer_array[i] = nullptr;
            }
        }
        
        delete []timer_array;
    }
    timer_heap(const timer_heap& th) = delete;
    timer_heap& operator=(const timer_heap& th) = delete;
public:
    bool empty() const { return cur_size_ == 0; }
    heap_timer* top() const { return empty() ? nullptr : timer_array[0]; }
    void pop();

    void add_timer(heap_timer* timer);
    void del_timer(heap_timer* timer);
    
    void tick();
private:
    void heapjust(int index);

    void resize();

private:
    heap_timer **timer_array;
    int capacity_;   //最大定时器的数量
    int cur_size_;   //当前定时器的数量
};

#endif 