#include <coroutine>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <source_location>
#include <string_view>
#include <tuple>
#include <utility>

template <typename Type>
class channel
{
public:
    channel(std::size_t buffer_size = 0) : buffer_size_{buffer_size}
    {}
    struct async_recv
    {
        channel<Type>& channel_;
        bool await_ready() const
        {
            return !channel_.fifo_.empty();
        }
        void await_suspend(std::coroutine_handle<> handle)
        {
            handle_ = handle;
        }
        auto await_resume()
        {
            if (channel_.closed_ && channel_.fifo_.empty())
            {
                return std::make_tuple(Type{}, false);
            }
            --channel_.reader_;
            Type t = std::move(channel_.fifo_.front());
            channel_.fifo_.pop_front();
            return std::make_tuple(std::move(t), true);
        }
        std::coroutine_handle<> handle_;
    };
    async_recv recv()
    {
        ++reader_;
        return async_recv{*this};
    }

    struct async_send
    {
        channel<Type>& channel_;
        bool await_ready() const
        {
            return channel_.fifo_.size() < channel_.buffer_size_ + channel_.reader_;
        }
        void await_suspend(std::coroutine_handle<> handle)
        {
            handle_ = handle;
        }
        void await_resume()
        {}
        std::coroutine_handle<> handle_;
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

private:
    std::size_t buffer_size_;
    std::size_t reader_{0};
    std::list<Type> fifo_{};
    bool closed_{false};
};

struct continuable
{
    struct promise_type
    {
        continuable get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void return_void()
        {}
        void unhandled_exception()
        {}
    };
    std::coroutine_handle<promise_type> handle_;
};

class io_polling
{
};

continuable recv1(channel<int>& chan)
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

continuable recv2(channel<int>& chan)
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

continuable send(channel<int>& chan)
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
    auto rc1 = recv1(chan);
    auto rc2 = recv2(chan);
    auto sc = send(chan);
    std::cout << __LINE__ << ": resume recv1\n";
    rc1.handle_.resume();
    std::cout << __LINE__ << ": resume send\n";
    sc.handle_.resume();
    std::cout << __LINE__ << ": resume recv2\n";
    rc2.handle_.resume();
    std::cout << __LINE__ << ": resume send\n";
    sc.handle_.resume();
    std::cout << __LINE__ << ": resume recv1\n";
    rc1.handle_.resume();
    std::cout << __LINE__ << ": resume recv2\n";
    rc2.handle_.resume();
    std::cout << "auto coro handle destroy because promise_type::final_suspend return "
                 "std::suspend_never\n";
}
