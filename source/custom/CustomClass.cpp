//
// Created by Liam on 10/02/2026.
//

#include "CustomClass.hpp"
#include "DataDiff.hpp"

namespace Nes
{
    namespace Custom
    {
        CustomClass::CustomClass()
        {
            // std::cout << "Hello, World!" << std::endl;
            // RingBuffer<int> buffer(5);
            //
            // std::cout << "Buffer empty? " << (buffer.empty() ? "Yes" : "No") << std::endl;
            // std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;
            // try
            // {
            //     std::cout << "Trying to pop from empty buffer" << std::endl;
            //     buffer.pop();
            //     // If we get here, something went wrong
            //     std::cout << "Popped data from empty buffer" << std::endl;
            // }
            // catch (const std::out_of_range& e)
            // {
            //     // What we want to happen
            //     std::cout << "Tried to pop from buffer" << std::endl;
            //     std::cout << e.what() << std::endl;
            // }
            //
            // buffer.push(1);
            // std::cout << "Buffer empty? " << (buffer.empty() ? "Yes" : "No") << std::endl;
            // std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;
            //
            // buffer.push(2);
            // buffer.push(3);
            // buffer.push(4);
            // buffer.push(5);
            // std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;
            //
            // int poppedData = buffer.pop();
            //
            // buffer.print();
            //
            // buffer.push(6);
            // buffer.push(7);
            //
            // std::cout << "Buffer size: " << buffer.size() << std::endl;
            // std::cout << "Buffer capacity: " << buffer.capacity() << std::endl;
            // std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;
            // std::cout << "Buffer empty? " << (buffer.empty() ? "Yes" : "No") << std::endl;
            // buffer.print(); // This will print the contents of the buffer, which should be "1 2 3".
            //
            // poppedData = buffer.pop();
            // std::cout << "Popped data: " << poppedData << std::endl;
            // buffer.print();

            RingBuffer<std::vector<int>> buffer2(5);
            std::vector vec1 = {1, 2, 3};
            std::vector vec2 = {1, 2, 4};
            buffer2.push(vec1);
            buffer2.push(vec2);

            std::cout << "Buffer size: " << buffer2.size() << std::endl;
            std::cout << buffer2[0][0] << " " << buffer2[0][1] << " " << buffer2[0][2] << std::endl;

            const std::vector differences = DataDiff<int>::CompareFiles(buffer2[0], buffer2[1]);
            std::cout << "number of differences: " << differences.size() << std::endl;
            for (auto vec11 : differences)
            {
                std::cout << "Difference at index " << vec11.byteIndex << ": " << vec11.oldFileByte << std::endl;
            }
        }
    }
}