#include <coroutine>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <source_location>
#include <string_view>
#include <tuple>
#include <utility>

#include "lazy.hh"

template <typename Derived>
class IntrusiveNode
{
public:
    Derived* next = nullptr;
};

template <typename T>
class FIFOList
{
public:
    FIFOList() : head(nullptr), tail(nullptr)
    {}

    void push(T* newNode)
    {
        if (tail == nullptr)
        {
            head = newNode;
            tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }

    T* pop()
    {
        if (head == nullptr)
        {
            return nullptr;
        }

        T* elem = head;
        head = head->next;
        if (head == nullptr)
        {
            // The list becomes empty after the pop
            tail = nullptr;
        }

        return elem;
    }

    bool empty()
    {
        return head == nullptr;
    }

private:
    T* head;
    T* tail;
};

template <typename Type>
class channel
{
public:
    channel(std::size_t buffer_size = 0) : buffer_size_{buffer_size}
    {}
    struct async_recv : public IntrusiveNode<async_recv>
    {
        async_recv(channel<Type>& channel) : channel_{channel}
        {}

        bool await_ready() const
        {
            return !channel_.fifo_.empty();
        }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle)
        {
            handle_ = handle;
            channel_.receivers_.push(this);
            if (!channel_.senders_.empty())
            {
                auto send = channel_.senders_.pop();
                return send->handle_;
            }
            return std::noop_coroutine();
        }
        auto await_resume()
        {
            if (channel_.closed_ && channel_.fifo_.empty())
            {
                return std::make_tuple(Type{}, false);
            }
            Type t = std::move(channel_.fifo_.front());
            channel_.fifo_.pop_front();
            return std::make_tuple(std::move(t), true);
        }

        channel<Type>& channel_;
        std::coroutine_handle<> handle_{};
    };
    async_recv recv()
    {
        return async_recv{*this};
    }

    struct async_send : public IntrusiveNode<async_send>
    {
        async_send(channel<Type>& channel) : channel_{channel}
        {}

        bool await_ready() const
        {
            return channel_.full();
        }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle)
        {
            handle_ = handle;
            channel_.senders_.push(this);
            if (!channel_.receivers_.empty())
            {
                auto recv = channel_.receivers_.pop();
                return recv->handle_;
            }
            return std::noop_coroutine();
        }
        void await_resume()
        {}
        channel<Type>& channel_;
        std::coroutine_handle<> handle_{};
    };

    async_send send(const Type& type)
    {
        fifo_.push_back(type);
        return async_send{*this};
    }

    async_send send(Type&& type)
    {
        fifo_.push_back(std::move(type));
        return async_send{*this};
    }

    void close()
    {
        closed_ = true;
    }

    void sync_await()
    {
        while ((closed_ || !fifo_.empty()) && !receivers_.empty())
        {
            auto recv = receivers_.pop();
            recv->handle_.resume();
        }
        while ((closed_ || !full()) && !senders_.empty())
        {
            auto send = senders_.pop();
            send->handle_.resume();
        }
    }

private:
    bool full()
    {
        return fifo_.size() < buffer_size_;
    }

    std::size_t buffer_size_;
    FIFOList<async_recv> receivers_{};
    FIFOList<async_send> senders_{};
    std::list<Type> fifo_{};
    bool closed_{false};
};

std::lazy<void> recv1(channel<int>& chan)
{
    std::cout << "recv1: begin\n";
    while (true)
    {
        auto&& [a, ok] = co_await chan.recv();
        if (!ok)
            break;
        std::cout << "recv1: " << a << "\n";
    }
    std::cout << "recv1: end\n";
    co_return;
};

std::lazy<void> wrapper(channel<int>& chan)
{
    std::cout << "wrapper: begin\n";
    co_await recv1(chan);
    std::cout << "wrapper: end\n";
}

std::lazy<void> recv2(channel<int>& chan)
{
    std::cout << "recv2: begin\n";
    while (true)
    {
        auto&& [a, ok] = co_await chan.recv();
        if (!ok)
            break;
        std::cout << "recv2: " << a << "\n";
    }
    std::cout << "recv2: end\n";
    co_return;
};

std::lazy<void> send(channel<int>& chan)
{
    std::cout << "send: begin\n";
    std::cout << "send: 0\n";
    co_await chan.send(0);
    std::cout << "send: 1\n";
    co_await chan.send(1);
    std::cout << "send: 2\n";
    co_await chan.send(2);
    std::cout << "send: 3\n";
    co_await chan.send(3);
    std::cout << "send: close\n";
    chan.close();
    std::cout << "send: end\n";
    co_return;
};

int main()
{
    channel<int> chan{};
    auto rc1 = wrapper(chan);
    auto rc2 = recv2(chan);
    auto sc = send(chan);
    rc1.sync_await();
    rc2.sync_await();
    sc.sync_await();
    std::cout << __LINE__ << ": sync_await\n";
    chan.sync_await();
    std::cout << "auto coro handle destroy because promise_type::final_suspend return "
                 "std::suspend_never\n";
}
