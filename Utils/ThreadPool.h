#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "NoneCopy.h"
#include <iostream>
#include <thread>
#include <future>
#include <functional>
#include <queue>
#include <vector>

class ThreadPool : public NoneCopy
{
public:
    using Task = std::packaged_task<void()>;

    //继承基类NoneCopy就不需要写如下删除了
    //ThreadPool(const ThreadPool&) = delete;
    //ThreadPool& operator=(const ThreadPool&) = delete;

    static ThreadPool& instance()
    {
        static ThreadPool ins;
        return ins;
    }

    ~ThreadPool()
    {
        stop();
    }

    template<class F, typename... Args>
    auto commit(F&& f, Args&&... args) ->
        std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))>
    {
        using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));

        if(stop_.load()) return std::future<RetType>{};

        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<RetType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            tasks_.emplace([task]{(*task)();});
        }
        cv_lock_.notify_one();
        return ret;
    }

private:
    ThreadPool(unsigned int number = std::thread::hardware_concurrency())
        :stop_(false)
    {
        thread_num_ = number <= 1 ? 2 : number;

        start();
    }

    void start()
    {
        for(int i = 0; i < thread_num_; ++i)
        {
            pool_.emplace_back([this]()
            {
                while(!this->stop_.load())
                {
                    Task task;
                    {
                        std::unique_lock<std::mutex> cv_mt(cv_mt_);
                        this->cv_lock_.wait(cv_mt, [this]
                        {
                            return this->stop_.load() || !this->tasks_.empty();
                        });

                        if(this->tasks_.empty()) return;

                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    this->thread_num_--;
                    task();
                    this->thread_num_++;
                }
            });
        }
    }

    void stop()
    {
        stop_.store(true);
        cv_lock_.notify_all();
        for(auto& td : pool_)
        {
            if(td.joinable())
            {
                std::cout << "join thread" << td.get_id() << std::endl;
                td.join();
            }
        }
    }

private:
    std::mutex               cv_mt_;
    std::condition_variable  cv_lock_;
    std::atomic_bool         stop_;
    std::atomic_int          thread_num_;
    std::queue<Task>         tasks_;
    std::vector<std::thread> pool_;
};

#endif // THREADPOOL_H
