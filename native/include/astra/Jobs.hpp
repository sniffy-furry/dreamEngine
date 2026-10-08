#pragma once
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace astra {
class JobSystem {
public:
    explicit JobSystem(std::size_t workers=std::max<std::size_t>(1,std::thread::hardware_concurrency())) : stop_(false) {
        workers_.reserve(workers);
        for(std::size_t i=0;i<workers;i++) workers_.emplace_back([this]{ workerLoop(); });
    }
    ~JobSystem(){ shutdown(); }
    JobSystem(const JobSystem&)=delete; JobSystem& operator=(const JobSystem&)=delete;
    template<class F> auto submit(F&& f)->std::future<std::invoke_result_t<F>> {
        using R=std::invoke_result_t<F>; auto task=std::make_shared<std::packaged_task<R()>>(std::forward<F>(f)); auto fut=task->get_future();
        { std::lock_guard<std::mutex> lock(mutex_); tasks_.emplace([task]() mutable {(*task)();}); }
        cv_.notify_one(); return fut;
    }
    void waitIdle(){ std::unique_lock<std::mutex> lock(mutex_); idleCv_.wait(lock,[this]{return tasks_.empty()&&active_==0;}); }
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_; std::condition_variable cv_,idleCv_; bool stop_; std::size_t active_=0;
    void workerLoop(){
        for(;;){ std::function<void()> task; { std::unique_lock<std::mutex> lock(mutex_); cv_.wait(lock,[this]{return stop_||!tasks_.empty();}); if(stop_&&tasks_.empty())return; task=std::move(tasks_.front()); tasks_.pop(); ++active_; }
            task(); { std::lock_guard<std::mutex> lock(mutex_); --active_; if(tasks_.empty()&&active_==0)idleCv_.notify_all(); }
        }
    }
    void shutdown(){ {std::lock_guard<std::mutex> lock(mutex_); stop_=true;} cv_.notify_all(); for(auto& t:workers_) if(t.joinable())t.join(); workers_.clear(); }
};
}
