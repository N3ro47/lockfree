#ifndef ATOMIC_SPINLOCK_QUEUE
#define ATOMIC_SPINLOCK_QUEUE

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
class AtomicSpinlockQueue
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
    AtomicSpinlockQueue()
    {
        initialize();
    }

    ~AtomicSpinlockQueue() = default;

    AtomicSpinlockQueue(const AtomicSpinlockQueue &) = delete;
    AtomicSpinlockQueue &operator=(const AtomicSpinlockQueue &) = delete;

    void enqueue(const T value)
    {
        auto new_node = std::make_shared<Node<T>>(value);
        std::shared_ptr<Node> last;
        while (true)
        {
            last = tail_.load(std::memory_order_acquire);
            auto next = last->next.load(std::memory_order_acquire);
            auto current_tail = tail_.load(std::memory_order_acquire); // Atomically read next
            if (last == current_tail)
            {
                if (!next)
                {
                    auto expected_next = next; // here idk if just take next or load again?
                    if (next.comapre_exchange_weak(expected_next, new_node, std::memory_order_acq_rel, std::memory_order_acquire))
                    {
                        auto expected_tail = last;
                        tail_.compare_exchange_strong(expected_tail, new_node, std::memory_order_release, std::memory_order_relaxed) break;
                    }
                }
                else
                {
                    auto expected_tail = last;
                    tail_.compare_exchange_strong(expected_tail, next, std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }
    }
// start from here rewriting to nower standard
    bool dequeue(T *pvalue)
    {
        auto current_head = head_.load();
        while (true)
        {
            auto next = current_head->next.load(); // Atomically read next
            if (!next)
            {
                // Queue is empty
                return false;
            }
            if (pvalue)
            {
                *pvalue = next->value;
            }
            // Try to move head to next
            if (head_.compare_exchange_weak(current_head, next))
            {
                return true;
            }
            // If CAS failed, current_head is updated, retry
        }
    }
};

#endif // ATOMIC_SPINLOCK_QUEUE