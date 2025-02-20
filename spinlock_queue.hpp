#ifndef ATOMIC_SPINLOCK_QUEUE
#define ATOMIC_SPINLOCK_QUEUE

#include "spinlock.hpp"

template <typename T>
struct Node {
    T value;
    Node<T>* next;

    Node() : next(nullptr) {}
    Node(const T val) : value(val), next(nullptr) {}
};

template <typename T>
class AtomicSpinlockQueue {
private:
    Node<T>* head_;
    Node<T>* tail_;
    spinlock h_lock_; // head node lock
    spinlock t_lock_; // head node lock

    void initialize() {
        Node<T>* node = new Node<T>(); // allocate initial node
        head_ = tail_ = node;          // Head and Tail point to it
    }

public:
    AtomicSpinlockQueue() {
        initialize(); // Initialize in constructor
    }

    ~AtomicSpinlockQueue() {
        Node<T>* current = head_;
        while (current != nullptr) {
            Node<T>* next = current->next;
            if constexpr (std::is_pointer<T>::value) {
                // If T is a pointer, delete the object pointed to by value
                delete current->value;
            }
            delete current;
            current = next;
        }
    }

    AtomicSpinlockQueue(const AtomicSpinlockQueue&) = delete;
    AtomicSpinlockQueue& operator=(const AtomicSpinlockQueue&) = delete;

    void enqueue(const T value) {
        Node<T>* node = new Node<T>(value); // Allocate a new node

        t_lock_.lock(); // Acquire T lock
        tail_->next = node; // Link node at the end of the linked list
        tail_ = node;     // Swing Tail to node
        t_lock_.unlock(); // Release T lock
    }

    bool dequeue(T* pvalue) {
        h_lock_.lock(); // Acquire head lock
        Node<T>* node = head_; // read Head
        Node<T>* new_head = node->next; // Read next pointer

        if (new_head == nullptr) { // Is queue empty? (only dummy node left)
            h_lock_.unlock(); // Release head lock
            return false; // Queue was empty
        }

        if (pvalue != nullptr) {
            *pvalue = new_head->value; // Queue not empty. Read value
        }

        head_ = new_head;     // Swing Head to next node
        h_lock_.unlock();     // Release head lock

        delete node; // Free old head node (dummy or actual first node)
        return true;    // Queue was not empty, dequeue succeeded
    }
};

#endif //ATOMIC_SPINLOCK_QUEUE