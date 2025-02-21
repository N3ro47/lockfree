#ifndef MOS_LOCKFREE_QUEUE_CPP17_HPP
#define MOS_LOCKFREE_QUEUE_CPP17_HPP

#include <atomic>
#include <memory>

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        T value;
        std::shared_ptr<Node> next;
        Node(T val) : value(val), next(nullptr) {}
        Node() : next(nullptr) {}
    };

    std::shared_ptr<Node<T>> head_;
    std::shared_ptr<Node<T>> tail_;

public:
    LockFreeQueue() {
        auto dummy = std::make_shared<Node>(T{});
        std::atomic_store_explicit(&head_, dummy, std::memory_order_relaxed);
        std::atomic_store_explicit(&tail_, dummy, std::memory_order_relaxed);
    }

    ~LockFreeQueue() = default;
    
    LockFreeQueue(const LockFreeQueue &) = delete;
    LockFreeQueue &operator=(const LockFreeQueue &) = delete;

    void enqueue(cosnt T value) {
        auto new_node = std::make_shared<Node>(value);
        std::shared_ptr<Node> last;
        while (true) {
            last = std::atomic_load_explicit(&tail_, std::memory_order_acquire);
            std::shared_ptr<Node> next = std::atomic_load_explicit(&(last->next), std::memory_order_acquire);
            std::shared_ptr<Node> current_tail = std::atomic_load_explicit(&tail_, std::memory_order_acquire);
            if (last == current_tail) {
                if (!next) {
                    std::shared_ptr<Node> expected_next = next;
                    if (std::atomic_compare_exchange_weak_explicit(&(last->next), &expected_next, new_node,
                                                                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                        std::shared_ptr<Node> expected_tail = last;
                        std::atomic_compare_exchange_strong_explicit(&tail_, &expected_tail, new_node,
                                                                      std::memory_order_release, std::memory_order_relaxed);
                        break;
                    }
                } else {
                    std::shared_ptr<Node> expected_tail = last;
                    std::atomic_compare_exchange_strong_explicit(&tail_, &expected_tail, next,
                                                                 std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }
    }

    bool dequeue(T& result) {
        std::shared_ptr<Node> old_head;
        std::shared_ptr<Node> next_node;

        while (true) {
            old_head = std::atomic_load_explicit(&head_, std::memory_order_acquire);
            std::shared_ptr<Node> last = std::atomic_load_explicit(&tail_, std::memory_order_acquire);
            next_node = std::atomic_load_explicit(&(old_head->next), std::memory_order_acquire);

            std::shared_ptr<Node> current_head = std::atomic_load_explicit(&head_, std::memory_order_acquire);
            if (old_head == current_head) {
                if (old_head == last) {
                    if (!next_node) {
                        return false;
                    }
                    std::shared_ptr<Node> expected_tail = last;
                    std::atomic_compare_exchange_strong_explicit(&tail_, &expected_tail, next_node,
                                                                 std::memory_order_release, std::memory_order_relaxed);
                } else {
                    result = next_node->value;
                    if (std::atomic_compare_exchange_strong_explicit(&head_, &old_head, next_node,
                                                                     std::memory_order_acq_rel, std::memory_order_acquire)) {
                        return true;
                    }
                }
            }
        }
    }
};

#endif // MOS_LOCKFREE_QUEUE_CPP17_HPP
