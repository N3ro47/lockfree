#ifndef DP_LOCKFREE_QUEUE_HPP
#define DP_LOCKFREE_QUEUE_HPP

#include <atomic>
#include <memory>

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        T value;
        std::shared_ptr<Node> next;
        Node(T val) : value(val), next(nullptr) {}
    };

    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;

public:
    LockFreeQueue() {
        auto dummy = std::make_shared<Node>(T{});
        std::atomic_store_explicit(&head, dummy, std::memory_order_relaxed);
        std::atomic_store_explicit(&tail, dummy, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        T value;
        while (dequeue(value)) {}
    }

    void enqueue(T value) {
        auto newNode = std::make_shared<Node>(value);
        std::shared_ptr<Node> last;
        while (true) {
            last = std::atomic_load_explicit(&tail, std::memory_order_acquire);
            std::shared_ptr<Node> next = std::atomic_load_explicit(&(last->next), std::memory_order_acquire);
            std::shared_ptr<Node> current_tail = std::atomic_load_explicit(&tail, std::memory_order_acquire);
            if (last == current_tail) {
                if (!next) {
                    std::shared_ptr<Node> expected_next = next;
                    if (std::atomic_compare_exchange_weak_explicit(&(last->next), &expected_next, newNode,
                                                                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                        std::shared_ptr<Node> expected_tail = last;
                        std::atomic_compare_exchange_strong_explicit(&tail, &expected_tail, newNode,
                                                                      std::memory_order_release, std::memory_order_relaxed);
                        break;
                    }
                } else {
                    std::shared_ptr<Node> expected_tail = last;
                    std::atomic_compare_exchange_strong_explicit(&tail, &expected_tail, next,
                                                                 std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }
    }

    bool dequeue(T& result) {
        std::shared_ptr<Node> old_head;
        std::shared_ptr<Node> next_node;

        while (true) {
            old_head = std::atomic_load_explicit(&head, std::memory_order_acquire);
            std::shared_ptr<Node> last = std::atomic_load_explicit(&tail, std::memory_order_acquire);
            next_node = std::atomic_load_explicit(&(old_head->next), std::memory_order_acquire);

            std::shared_ptr<Node> current_head = std::atomic_load_explicit(&head, std::memory_order_acquire);
            if (old_head == current_head) {
                if (old_head == last) {
                    if (!next_node) {
                        return false;
                    }
                    std::shared_ptr<Node> expected_tail = last;
                    std::atomic_compare_exchange_strong_explicit(&tail, &expected_tail, next_node,
                                                                 std::memory_order_release, std::memory_order_relaxed);
                } else {
                    result = next_node->value;
                    if (std::atomic_compare_exchange_strong_explicit(&head, &old_head, next_node,
                                                                     std::memory_order_acq_rel, std::memory_order_acquire)) {
                        return true;
                    }
                }
            }
        }
    }
};

#endif // DP_LOCKFREE_QUEUE_HPP