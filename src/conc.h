#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Conc {

template <typename T>
class VarL {
    private:
        T    val;
        std::mutex mutex_;

    public:
        VarL(T w) : val(w) {
            val = w;
        }
        template <typename FN>
        auto modify(FN&& fn) {
            std::lock_guard<std::mutex> mlock(mutex_);
            return fn(val);
        }
};

template <typename T>
class Chan {
    public:
        std::unique_ptr<T> pop() {
            std::unique_lock<std::mutex> mlock(mutex_);
            while (queue_.empty()) {
                cond_.wait(mlock);
            }
            std::unique_ptr<T> item = std::move(queue_.front());
            queue_.pop();
            return item;
        }

        void pop(T& item) {
            std::unique_lock<std::mutex> mlock(mutex_);
            while (queue_.empty()) {
                cond_.wait(mlock);
            }
            item = *(queue_.front());
            queue_.pop();
        }

        void push(const T& item) {
            std::unique_lock<std::mutex> mlock(mutex_);
            queue_.push(std::make_unique<T>(item));
            mlock.unlock();
            cond_.notify_one();
        }
/*
        void push(T&& item) {
            std::unique_lock<std::mutex> mlock(mutex_);
            queue_.push(std::move(item));
            mlock.unlock();
            cond_.notify_one();
        } */
 private:
  std::queue<std::unique_ptr<T>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};
// concurrent queue based on https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
}