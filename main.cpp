#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <cassert>
#include <mutex>

constexpr int NUM_PRODUCERS = 4;
constexpr int ITEMS_PER_THREAD = 10000000;

struct TestData {
    int producerId;
    int sequence;
};

#ifdef USE_SPINLOCK_QUEUE
#include "spinlock_queue.hpp"
using QueueType = SpinlockQueue<TestData>;
#elif defined(USE_LOCKFREE_QUEUE)
#include "lockfree_queue.hpp"
using QueueType = LockFreeQueue<TestData>;
#else
#error "No queue type defined. Define either USE_SPINLOCK_QUEUE or USE_LOCKFREE_QUEUE."
#endif

QueueType queue;
std::atomic<int> globalSequence{0};
std::atomic<bool> running{true};
std::mutex outputMutex;

void producer(int id) {
    for (int i = 0; i < ITEMS_PER_THREAD; ++i) {
        int seq = globalSequence.fetch_add(1, std::memory_order_relaxed);
        queue.enqueue( {id, seq}); 
    }
}

void consumer(std::vector<std::vector<int>>& receivedSequences) {
    TestData data;
    TestData* pdata = &data;
    while (true) {
        if (queue.dequeue(data)) {
            // Process item
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                receivedSequences[data.producerId].push_back(data.sequence);
            }
        } else if (!running.load(std::memory_order_acquire)) {
            //not running, drain any remaining items.
            while (queue.dequeue(data)) {
                std::lock_guard<std::mutex> lock(outputMutex);
                receivedSequences[data.producerId].push_back(data.sequence);
            }
            break;
        } else {
            std::this_thread::yield();
        }
    }
}


int main() {
    std::vector<std::vector<int>> receivedSequences(NUM_PRODUCERS);
    std::vector<std::thread> producers;

    // start producers
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer, i);
    }

    //   start consumer
    std::thread consumerThread(consumer, std::ref(receivedSequences));

    // Join producers
    for (auto& p : producers) p.join();
    std::cout << "finished writing\n";

    // Stop consumer
    running.store(false, std::memory_order_release);
    consumerThread.join();
    std::cout << "finished consuming\n";

    // Verify ordering
    int totalReceived = 0;
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        int lastSeq = -1;
        for (int seq : receivedSequences[i]) {
            assert(seq > lastSeq && "Error: Out-of-order sequence detected!");
            lastSeq = seq;
            totalReceived++;
        }
    }
    std::cout << totalReceived << std::endl;
    std::cout << NUM_PRODUCERS * ITEMS_PER_THREAD << std::endl;
    assert(totalReceived == NUM_PRODUCERS * ITEMS_PER_THREAD && "Error: Mismatch in number of items processed!");

    std::cout << "Test passed: queue maintains order and integrity.\n";
    return 0;
}