#ifndef LOCKFREE_QUEUE_HPP
#define LOCKFREE_QUEUE_HPP

#include <atomic>
#include <memory>

template <typename T>
class lockfree_queue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next{nullptr};

        Node(const T& d) : data(d) {}
    };

    std::atomic<Node*> head{new Node(T{})};
    std::atomic<Node*> tail{head.load()};

public:
    // Queue implementation uses a dummy node as its head
    lockfree_queue() : head(new Node(T{})), tail(head.load()) {
        head.load()->next.store(nullptr);
    }

    ~lockfree_queue() {
        Node* current = head.load();
        while (current) {
            Node* next = current->next.load();
            delete current;
            current = next;
        }
    }

    bool enqueue(const T& data) {
        std::unique_ptr<Node> newNode(new (std::nothrow) Node(data));
        if (!newNode) return false;

        unsigned int attempts = 0;
        constexpr unsigned int maxAttempts = 1000; //deffend frome livelock

        while (attempts++ < maxAttempts) {
            Node* oldTail = tail.load(std::memory_order_acquire);
            if (!oldTail) return false;  // ensure tail is non-null
            Node* oldNext = oldTail->next.load(std::memory_order_acquire);

            if (oldTail == tail.load(std::memory_order_acquire)) { //check if it's the same 
                if (oldNext == nullptr) { // check if it's the last one 
                    if (oldTail->next.compare_exchange_weak(oldNext, newNode.get(), std::memory_order_release, std::memory_order_acquire)) { //point at 
                        tail.compare_exchange_strong(oldTail, newNode.release(), std::memory_order_release, std::memory_order_acquire); //strong bcs we already changed the oldtail.next prt
                        return true;
                    }
                } else {
                    tail.compare_exchange_weak(oldTail, oldNext, std::memory_order_release, std::memory_order_acquire);// read new tail
                }
            }
        }
        return false;  // Too many retries
    }

    bool dequeue(T& result) {
        // The first node added is a dummy node, and when dequeuing, we read the data
        // from the node immediately after the dummy node (head->next) and then update
        // the head pointer to point to that node.
        unsigned int attempts = 0;
        constexpr unsigned int maxAttempts = 1000; //deffend frome livelock

        while(attempts++ < maxAttempts) {
            Node* oldTail = tail.load(std::memory_order_acquire);
            Node* oldHead = head.load(std::memory_order_acquire);
            if(!oldHead) return false; // ensure head is non-null
            Node* oldNext = oldHead->next.load(std::memory_order_acquire);

            if (oldHead == head.load(std::memory_order_acquire)) { //check if it's the same 
                if (oldHead == oldTail) { 
                    if (oldNext == nullptr)
                        return false; // Queue is empty
                    tail.compare_exchange_weak(oldTail, oldNext, std::memory_order_release, std::memory_order_acquire);
                } else {
                    result = oldNext->data; //read data
                    if (head.compare_exchange_weak(oldHead, oldNext, std::memory_order_release, std::memory_order_acquire)) { // try to change the next head to 
                        delete oldHead; //the first node is a dummy node.
                        return true;
                    }
                }
            }
        }
        return false; // Too many retries
    }
};

#endif // LOCKFREE_QUEUE_HPP
