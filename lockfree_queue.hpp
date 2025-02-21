#ifndef MOS_LOCKFREE_QUEUE_HPP
#define MOS_LOCKFREE_QUEUE_HPP

#include <atomic>
#include <memory>

template <typename T>
struct Node
{
    T value;
    std::atomic<std::shared_ptr<Node<T>>> next;

    Node() : next(nullptr) {}
    Node(const T val) : value(val), next(nullptr) {}
};

template <typename T>
class LockFreeQueue
{
private:
    std::atomic<std::shared_ptr<Node<T>>> head_;
    std::atomic<std::shared_ptr<Node<T>>> tail_;

    void initialize()
    {
        auto dummy = std::make_shared<Node<T>>();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

public:
    LockFreeQueue()
    {
        initialize();
    }

    ~LockFreeQueue() = default;

    LockFreeQueue(const LockFreeQueue &) = delete;
    LockFreeQueue &operator=(const LockFreeQueue &) = delete;

    void enqueue(const T value)
    {
        auto new_node = std::make_shared<Node<T>>(value);
        std::shared_ptr<Node<T>> last;
        while (true)
        {
            last = tail_.load(std::memory_order_acquire);
            auto next = last->next.load(std::memory_order_acquire);
            auto current_tail = tail_.load(std::memory_order_acquire);
            if (last == current_tail)
            {
                if (!next)
                {
                    auto expected_next = next;
                    if (last->next.compare_exchange_weak(expected_next, new_node, 
                                                         std::memory_order_acq_rel, 
                                                         std::memory_order_acquire))
                    {
                        auto expected_tail = last;
                        tail_.compare_exchange_strong(expected_tail, new_node, 
                                                      std::memory_order_release, 
                                                      std::memory_order_relaxed);
                        break;
                    }
                }
                else
                {
                    auto expected_tail = last;
                    tail_.compare_exchange_strong(expected_tail, next, 
                                                  std::memory_order_release, 
                                                  std::memory_order_relaxed);
                }
            }
        }
    }

    bool dequeue(T& value)
    {
        while (true)
        {
            auto old_head = head_.load(std::memory_order_acquire);
            auto last = tail_.load(std::memory_order_acquire);
            auto next_node = old_head->next.load(std::memory_order_acquire);
            auto current_head = head_.load(std::memory_order_acquire);
            if (old_head == current_head)
            {
                if (old_head == last)
                {
                    if (!next_node)
                    {
                        return false;
                    }
                    auto expected_tail = last;
                    tail_.compare_exchange_strong(expected_tail, next_node, 
                                                  std::memory_order_release, 
                                                  std::memory_order_relaxed);
                }
                else
                {
                    value = next_node->value;
                    if (head_.compare_exchange_strong(old_head, next_node, 
                                                      std::memory_order_acq_rel, 
                                                      std::memory_order_acquire))
                    {
                        return true;
                    }
                }
            }
        }
    }
};

#endif // MOS_LOCKFREE_QUEUE_HPP
