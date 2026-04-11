//
// Created by Liam on 12/02/2026.
// I decided to write this all in a single header file since it's a simple class, and it avoids the need for multiple files.
// It also means that this struct can be easily included in other projects as a standalone component.
//

#ifndef NESTOPIA_RINGBUFFER_HPP
#define NESTOPIA_RINGBUFFER_HPP

// Although this is a standalone class, it does require the vector class from the C++ standard library
// to manage the underlying storage of the buffer.


// This Ring Buffer is a simple implementation of a circular buffer that can
// store any type of data using a Type template. It has a fixed capacity and
// overwrites the oldest data when new data is added beyond the capacity.
// The structure uses FIFO (First In, First Out) order to overwrite data, meaning that
// the oldest data is removed first when the buffer is full
// and LIFO (Last in, First out) order to pop data, meaning that the most recently
// added data is removed first when popping from the buffer.
template <typename T>
struct RingBuffer
{
    // Initialises the ring buffer with a specified capacity.
    explicit RingBuffer(const int capacity) : buffer(capacity), _capacity(capacity), head(0), tail(0), _size(0) {}
    ~RingBuffer() = default;

    // Returns the current size of the buffer.
    [[nodiscard]] int size() const { return _size; }
    [[nodiscard]] int capacity() const { return _capacity; }

    [[nodiscard]] bool empty() const { return _size == 0; }
    [[nodiscard]] bool full() const { return _size == _capacity; }

    void push(T data)
    {
        // we need to make a way to copy a vector into a vector.
        buffer[tail] = std::move(data); // std::move here for O(1)
        tail = (tail + 1) % _capacity; // This increases the tail's position, wrapping around if it goes over the capacity
        if (_size < _capacity) _size++; // check if we're over the capacity
        else head = (head + 1) % _capacity; // if we are, we need to evict the oldest
    }

    T pop()
    {
        if (_size == 0) throw std::out_of_range("Empty buffer");
        tail = (tail - 1 + _capacity) % _capacity; // Move tail back by one, wrapping around if negative
        std::cout << buffer[tail] << ": Tail";
        _size--; // Shrink the size by one
        return std::move(buffer[tail]); // std::move for O(1)
    }

    // Prints out the list of *active* elements in the vector
    void print()
    {
        // Start at the tail, AKA the least recent element
        int index = tail;

        // Iterate through the array for how big it is currently
        for (int i = 0; i < _size; i++)
        {
            // make the index to print the index + 1 MOD the capacity
            // to ensure that we can wrap properly
            index = (index + 1) % _capacity;
            // print out the buffer at the index
            std::cout << buffer[index];
        }
        std::cout << std::endl;
    }

    T operator[](const int index)
    {
        return buffer[(head + index) % _capacity]; // This allows us to access the buffer like an array, with the tail as the starting point
    }

private:
    std::vector<T> buffer; // the actual buffer
    int _capacity,  // Overall capacity of the buffer
        head,       // The position of the head
        tail,       // The position of the tail
        _size;      // The current size of the buffer
};

#endif //NESTOPIA_RINGBUFFER_HPP