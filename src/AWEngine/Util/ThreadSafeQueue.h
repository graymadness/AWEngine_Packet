#pragma once

#include <mutex>
#include <deque> // double-ended <queue>
#include <condition_variable>

namespace AWEngine::Util
{
    template<typename T>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue() = default;

        ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;

    public:
        ~ThreadSafeQueue()
        {
            clear();
        }

    private:
        /// Read/Write mutex
        std::mutex m_MutesQueue = {};
        /// Current data in the queue
        /// Using std::queue as it is part of standard libraries
        std::deque<T> m_Data = {};

    private:
        /// Gets set when item is pushed
        /// For wait()
        std::condition_variable m_ItemPushed;
        /// Mutex lock for wait()
        /// Locked in push_*() procedures
        std::mutex m_MutexItemPushed = {};

    public:
        /// Read the first item without removing it from the queue
        const T& peek_front()
        {
            std::scoped_lock lock(m_MutesQueue);
            return m_Data.front();
        }
        /// Read the last item without removing it from the queue
        const T& peek_back()
        {
            std::scoped_lock lock(m_MutesQueue);
            return m_Data.back();
        }

    public:
        /// Read the first item and remove it from the queue
        T pop_front()
        {
            std::scoped_lock lock(m_MutesQueue);
            auto t = std::move(m_Data.front());
            m_Data.pop_front();
            return std::move(t);
        }
        /// Read the last item and remove it from the queue
        T pop_back()
        {
            std::scoped_lock lock(m_MutesQueue);
            auto t = std::move(m_Data.back());
            m_Data.pop_back();
            return std::move(t);
        }

    public:
        /// Add item to start
        void push_front(const T& item)
        {
            std::scoped_lock lock(m_MutesQueue);
            m_Data.emplace_front(std::move(item));

            std::unique_lock<std::mutex> ul(m_MutexItemPushed);
            m_ItemPushed.notify_one();
        }
        /// Add item to end
        void push_back(const T& item)
        {
            std::scoped_lock lock(m_MutesQueue);
            m_Data.emplace_back(std::move(item));

            std::unique_lock<std::mutex> ul(m_MutexItemPushed);
            m_ItemPushed.notify_one();
        }

    public:
        /// Check whenever there are any items
        bool empty()
        {
            std::scoped_lock lock(m_MutesQueue);
            return m_Data.empty();
        }
        /// Number of items in the queue
        size_t size()
        {
            std::scoped_lock lock(m_MutesQueue);
            return m_Data.size();
        }
        /// Remove all items
        void clear()
        {
            std::scoped_lock lock(m_MutesQueue);
            m_Data.clear();
        }

    public:
        /// Wait for call of push_*()
        /// Blocks current thread
        void wait()
        {
            while (empty())
            {
                std::unique_lock<std::mutex> ul(m_MutexItemPushed);
                m_ItemPushed.wait(ul);
            }
        }
    };
}
