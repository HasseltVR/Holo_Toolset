/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
#ifndef THREADSAFECONTAINERS_H
#define THREADSAFECONTAINERS_H

#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <map>

/*
 * ThreadSafe Queue: allows for threadsafe adding elements to a FIFO queue. It also allows for multiple
 * threads to process items for the queue without having race conditions
 *
 */
template<typename T>
class ThreadSafe_Queue
{
private:
    mutable std::mutex mtx;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    ThreadSafe_Queue() {}
    ThreadSafe_Queue(ThreadSafe_Queue const& other)
    {
        std::lock_guard<std::mutex> lock(other.mtx);
        data_queue=other.data_queue;
    }

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(mtx);
        data_queue.push(new_value);
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(mtx);
        data_cond.wait(lock,[this]{return !data_queue.empty();});
        value=data_queue.front();
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        data_cond.wait(lock,[this]{return !data_queue.empty();});
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(data_queue.empty())
            return false;
        value=data_queue.front();
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return data_queue.empty();
    }

    std::size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return data_queue.size();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::queue<T> empty;
        std::swap( data_queue, empty );
    }
};

/*
* ThreadSafe Map: allows for threadsafe adding elements to a map. It also allows for multiple
* threads to process items for the queue without having race conditions.
* Item retrieval is done by using a unique key, unlike the above FIFO queue.
* The map internally ensures that all items are sorted, with the lowest key first.
*
*/
template<typename T>
class ThreadSafe_Map
{
private:
    mutable std::mutex mtx;
    std::map<uint64_t, T> data_queue;
    std::condition_variable data_cond;

public:
    ThreadSafe_Map() {}
    ThreadSafe_Map(ThreadSafe_Map const& other)
    {
        std::lock_guard<std::mutex> lock(other.mtx);
        data_queue = other.data_queue;
    }

    void push(T new_value, uint64_t key)
    {
        std::lock_guard<std::mutex> lock(mtx);
        data_queue.insert(std::pair<uint64_t, T>(key, new_value));
        data_cond.notify_one();
    }

    void wait_and_pop(T& value, uint64_t key)
    {
        std::unique_lock<std::mutex> lock(mtx);
        data_cond.wait(lock, [this, key] {return data_queue.count(key); });
        value = data_queue.at(key);
        data_queue.erase(key);
    }

    std::shared_ptr<T> wait_and_pop(uint64_t key)
    {
        std::unique_lock<std::mutex> lock(mtx);
        data_cond.wait(lock, [this, key] {return data_queue.count(key); });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.at(key)));
        data_queue.erase(key);
        return res;
    }

    bool try_pop(T& value, uint64_t key)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (data_queue.empty())
            return false;
        value = data_queue.at(key);
        data_queue.erase(key);
        return true;
    }

    std::shared_ptr<T> try_pop(uint64_t key)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.at(key)));
        data_queue.erase(key);
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return data_queue.empty();
    }

    std::size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return data_queue.size();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mtx);
        data_queue.clear();
    }
};

#endif // THREADSAFECONTAINERS_H
