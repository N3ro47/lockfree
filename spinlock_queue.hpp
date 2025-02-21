#ifndef MOS_SPINLOCK_QUEUE
#define MOS_SPINLOCK_QUEUE

#include "spinlock.hpp"

template <typename T>
struct Node {
    T value;
    Node<T>* next;

    Node() : next(nullptr) {}
    Node(const T val) : value(val), next(nullptr) {}
};

template <typename T>
class SpinlockQueue {
private:
    Node<T>* head_;
    Node<T>* tail_;
    spinlock h_lock_; // head node lock
    spinlock t_lock_; // tail node lock

    void initialize() {
        Node<T>* node = new Node<T>(); // allocate initial node
        head_ = tail_ = node;          // Head and Tail point to it
    }

public:
    SpinlockQueue() {
        initialize(); // Initialize in constructor
    }

    ~SpinlockQueue() {
        Node<T>* current = head_;
        while (current != nullptr) {
            Node<T>* next = current->next;
            delete current;
            current = next;
        }
    }

    SpinlockQueue(const SpinlockQueue&) = delete;
    SpinlockQueue& operator=(const SpinlockQueue&) = delete;

    void enqueue(const T value) {
        Node<T>* node = new Node<T>(value); // Allocate a new node

        t_lock_.lock(); // Acquire T lock
        tail_->next = node; // Link node at the end of the linked list
        tail_ = node;     // Swing Tail to node
        t_lock_.unlock(); // Release T lock
    }

    bool dequeue(T& value) {
        h_lock_.lock(); // Acquire head lock
        Node<T>* node = head_; // read Head
        Node<T>* new_head = node->next; // Read next pointer

        if (new_head == nullptr) { // Is queue empty? (only dummy node left)
            h_lock_.unlock(); // Release head lock
            return false; // Queue was empty
        }

        value = new_head->value; // Copy value to output parameter
        head_ = new_head;     // swing head
        h_lock_.unlock();     // Release head lock

        delete node; // Free old head node (dummy or actual first node)
        return true;    // Queue was not empty, dequeue succeeded
    }
};

#endif // MOS_SPINLOCK_QUEUE
