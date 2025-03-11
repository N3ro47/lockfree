# Lock-Free/Atomics Queue Project

## Build Instructions

This project provides a queue implementation with two options: a lock-free queue and a spinlock-based queue. You can choose which one to use at compile time.

### Requirements
- A C++ compiler supporting C++20 g++
- `make`
- POSIX Threads (`pthread`)

### Compilation
To compile the project, use the following command:

```sh
make
```
This will compile the project using the default spinlock queue.

#### Selecting the Lock-Free Queue
If you want to use the lock-free queue implementation, specify `QUEUE=lockfree`:

```sh
make QUEUE=lockfree
```

### Running the Program
Once compiled, you can run the program by executing:

```sh
./queue_test
```

### Cleaning the Build
To remove the compiled binary, run:

```sh
make clean
```

