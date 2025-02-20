#ifndef SA_LOCKFREE_QUEUE_HPP
#define SA_LOCKFREE_QUEUE_HPP

#include <atomic>
#include <memory>

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        T value;
        std::atomic<std::shared_ptr<Node>> next;
        Node(T val) : value(val), next(nullptr) {}
    };
    
    std::atomic<std::shared_ptr<Node>> head;
    std::atomic<std::shared_ptr<Node>> tail;

public:
    LockFreeQueue() {
        auto dummy = std::make_shared<Node>(T{}); // Dummy node
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }

    void enqueue(T value) {
        auto newNode = std::make_shared<Node>(value);
        std::shared_ptr<Node> last;
        while (true) {
            last = tail.load(std::memory_order_acquire);
            auto next = last->next.load(std::memory_order_acquire);
            if (last == tail.load(std::memory_order_acquire)) {
                if (!next) {
                    if (last->next.compare_exchange_strong(next, newNode, std::memory_order_release, std::memory_order_relaxed)) {
                        tail.compare_exchange_strong(last, newNode, std::memory_order_release, std::memory_order_relaxed);
                        break;
                    }
                } else {
                    tail.compare_exchange_strong(last, next, std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }
    }

    bool dequeue(T& result) {
        std::shared_ptr<Node> old_head;
        while (true) {
            old_head = head.load(std::memory_order_acquire);
            std::shared_ptr<Node> last = tail.load(std::memory_order_acquire);
            std::shared_ptr<Node> next = old_head->next.load(std::memory_order_acquire);
            
            if (old_head == head.load(std::memory_order_acquire)) {
                if (old_head == last) {
                    if (!next) return false;
                    tail.compare_exchange_strong(last, next, std::memory_order_release, std::memory_order_relaxed);
                } else {
                    if (next) result = next->value;
                    if (head.compare_exchange_strong(old_head, next, std::memory_order_acq_rel, std::memory_order_acquire)) {
                        return true;
                    }
                }
            }
        }
    }
};

#endif // SA_LOCKFREE_QUEUE_HPP
