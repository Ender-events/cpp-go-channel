#include <cassert>
#include <coroutine>
#include <cstddef>
#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <source_location>
#include <stdexcept>
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

    auto pop() -> T*
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

    auto empty() -> bool
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

        [[nodiscard]] auto await_ready() const -> bool
        {
            return !channel_.fifo_.empty() || !channel_.senders_.empty();
        }
        auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<>
        {
            handle_ = handle;
            channel_.receivers_.push(this);
            if (!channel_.consumeds_.empty())
            {
                auto send = channel_.consumeds_.pop();
                return send->handle_;
            }
            if (!channel_.senders_.empty())
            {
                auto send = channel_.senders_.pop();
                return send->handle_;
            }
            return std::noop_coroutine();
        }
        auto await_resume()
        {
            if (!channel_.fifo_.empty())
            {
                Type data = std::move(channel_.fifo_.front());
                channel_.fifo_.pop_front();
                return std::make_tuple(std::move(data), true);
            }
            if (!channel_.senders_.empty())
            {
                auto send = channel_.senders_.pop();
                Type data = std::move(send->data_.value());
                send->data_.reset();
                channel_.consumeds_.push(send);
                return std::make_tuple(std::move(data), true);
            }
            if (!channel_.closed_)
            {
                throw std::runtime_error("unexpected await resume");
            }
            return std::make_tuple(Type{}, false);
        }

        channel<Type>& channel_;
        std::coroutine_handle<> handle_{};
    };
    auto recv() -> async_recv
    {
        return async_recv{*this};
    }

    struct async_send : public IntrusiveNode<async_send>
    {
        async_send(channel<Type>& channel, Type&& data) : channel_{channel}, data_{std::move(data)}
        {}

        auto await_ready() -> bool
        {
            if (channel_.full())
            {
                return false;
            }
            channel_.fifo_.push_back(std::move(data_.value()));
            data_.reset();
            return true;
        }
        auto await_suspend(std::coroutine_handle<> handle) -> std::coroutine_handle<>
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
        std::optional<Type> data_;
        std::coroutine_handle<> handle_{};
    };

    auto send(const Type& type) -> async_send
    {
        return async_send{*this, Type{type}};
    }

    auto send(Type&& type) -> async_send
    {
        return async_send{*this, std::move(type)};
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
    auto full() -> bool
    {
        return fifo_.size() >= buffer_size_;
    }

    std::size_t buffer_size_;
    FIFOList<async_recv> receivers_{};
    FIFOList<async_send> senders_{};
    FIFOList<async_send> consumeds_{};
    std::deque<Type> fifo_{};
    bool closed_{false};
};

auto recv1(std::shared_ptr<channel<int>> chan) -> std::lazy<void>
{
    std::cout << "recv1: begin\n";
    while (true)
    {
        auto&& [a, ok] = co_await chan->recv();
        if (!ok)
        {
            break;
        }
        std::cout << "recv1: " << a << "\n";
    }
    std::cout << "recv1: end\n";
    co_return;
};

auto wrapper(std::shared_ptr<channel<int>> chan) -> std::lazy<void>
{
    std::cout << "wrapper: begin\n";
    co_await recv1(chan);
    std::cout << "wrapper: end\n";
}

auto recv2(std::shared_ptr<channel<int>> chan) -> std::lazy<void>
{
    std::cout << "recv2: begin\n";
    while (true)
    {
        auto&& [a, ok] = co_await chan->recv();
        if (!ok)
        {
            break;
        }
        std::cout << "recv2: " << a << "\n";
    }
    std::cout << "recv2: end\n";
    co_return;
};

auto send(std::shared_ptr<channel<int>> chan) -> std::lazy<void>
{
    std::cout << "send: begin\n";
    std::cout << "send: 0\n";
    co_await chan->send(0);
    std::cout << "send: 1\n";
    co_await chan->send(1);
    std::cout << "send: 2\n";
    co_await chan->send(2);
    std::cout << "send: 3\n";
    co_await chan->send(3);
    std::cout << "send: close\n";
    chan->close();
    std::cout << "send: end\n";
    co_return;
};

auto main() -> int
{
    auto chan = std::make_shared<channel<int>>();
    auto rc1 = wrapper(chan);
    auto rc2 = recv2(chan);
    auto sc = send(chan); // NOLINT(readability-identifier-length)
    std::cout << "rc1::sync_await\n";
    rc1.sync_await();
    std::cout << "rc2::sync_await\n";
    rc2.sync_await();
    std::cout << "sc::sync_await\n";
    sc.sync_await();
    std::cout << __LINE__ << ": sync_await\n";
    chan->sync_await();
    std::cout << "auto coro handle destroy because promise_type::final_suspend return "
                 "std::suspend_never\n";
}
