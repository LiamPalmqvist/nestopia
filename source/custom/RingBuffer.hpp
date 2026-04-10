//
// Created by Liam on 12/02/2026.
// I decided to write this all in a single header file since it's a simple class, and it avoids the need for multiple files.
// It also means that this struct can be easily included in other projects as a standalone component.
//

#ifndef NESTOPIA_RINGBUFFER_HPP
#define NESTOPIA_RINGBUFFER_HPP

// Although this is a standalone class, it does require the vector class from the C++ standard library
// to manage the underlying storage of the buffer.
#include <vector>


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
    explicit RingBuffer(const int capacity) : _capacity(capacity)
    {
        buffer.reserve(_capacity);
    }
    ~RingBuffer() = default;

    // Returns the current size of the buffer.
    [[nodiscard]] int size() const { return buffer.size(); }
    [[nodiscard]] int capacity() const { return _capacity; }

    bool full() const { return buffer.size() >= _capacity; }

    bool empty() const { return buffer.empty(); }

    void push(T data)
    {
        std::cout << "Pushing data: " << data << std::endl;
        // If the buffer is not full, add the new data.
        if (!full())
            buffer.push_back(data);
        // Otherwise, remove the oldest data and add the new data.
        else
        {
            std::cout << "Buffer is full" << std::endl;
            std::cout << "Removing oldest data: " << buffer.front() << std::endl;
            buffer.erase(buffer.begin());
            buffer.push_back(data);
        }
        std::cout << "Buffer size: " << buffer.size() << std::endl;
    }

    T pop() {
        if (buffer.empty()) throw std::runtime_error("pop() called on empty buffer");
        T data = buffer.back();
        buffer.pop_back();
        return data;
    }

    void print() const
    {
        if (buffer.empty()) return;
        for (const T item : buffer)
        {
            //std::cout << "Hello" << std::endl;
            std::cout << item << " ";
        }
    }

private:
    std::vector<T> buffer;
    int _capacity;
};

#endif //NESTOPIA_RINGBUFFER_HPP