CXX       = g++
CXXFLAGS  = -std=c++20 -O2 -pthread
TARGET    = queue_test
SRCS      = main.cpp

# Compile-time queue selection:
# To select the lock-free queue, run: make QUEUE=lockfree
ifeq ($(QUEUE), lockfree)
    CXXFLAGS += -DUSE_LOCKFREE_QUEUE
else
    CXXFLAGS += -DUSE_SPINLOCK_QUEUE
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
