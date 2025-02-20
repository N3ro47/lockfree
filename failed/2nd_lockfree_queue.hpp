#ifndef LOCKFREE_QUEUE_HPP
#define LOCKFREE_QUEUE_HPP

#include <atomic>

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        T value;
        std::atomic<Node*> next;
        Node(T val) : value(val), next(nullptr) {}
    };
    
    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    LockFreeQueue() {
        Node* dummy = new Node(T{}); // Dummy node
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        T value;
        while (dequeue(value));
        delete head.load(std::memory_order_relaxed);
    }

    void enqueue(T value) {
        Node* newNode = new Node(value);
        Node* last;
        while (true) {
            last = tail.load(std::memory_order_acquire);
            Node* next = last->next.load(std::memory_order_acquire);
            if (last == tail.load(std::memory_order_acquire)) {
                if (next == nullptr) {
                    if (last->next.compare_exchange_strong(next, newNode, std::memory_order_acq_rel, std::memory_order_acquire)) {
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
    Node* old_head;
    Node* next_node;

    while (true) {
        old_head = head.load(std::memory_order_acquire);
        Node* last = tail.load(std::memory_order_acquire);
        next_node = old_head->next.load(std::memory_order_acquire);

        if (old_head == head.load(std::memory_order_acquire)) {
            if (old_head == last) {
                if (next_node == nullptr) return false;
                tail.compare_exchange_strong(last, next_node, std::memory_order_release, std::memory_order_relaxed);
            } else {
                result = next_node->value;

                if (head.compare_exchange_strong(old_head, next_node, std::memory_order_acq_rel, std::memory_order_acquire)) {
                    delete old_head;
                    return true;
                } else {
                    continue;
                }
            }
        }
    }
}
};



#endif // LOCKFREE_QUEUE_HPP