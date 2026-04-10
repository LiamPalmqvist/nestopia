//
// Created by Liam on 10/02/2026.
//

#include "CustomClass.hpp"

namespace Nes
{
    namespace Custom
    {
        CustomClass::CustomClass()
        {
            std::cout << "Hello, World!" << std::endl;
            RingBuffer<int> buffer(5);

            std::cout << "Buffer empty? " << (buffer.empty() ? "Yes" : "No") << std::endl;
            std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;
            try
            {
                buffer.pop();
                // If we get here, something went wrong
                std::cout << "Popped data from empty buffer" << std::endl;
            }
            catch (const std::runtime_error& e)
            {
                // What we want to happen
                std::cout << e.what() << std::endl;
            }

            buffer.push(1);
            std::cout << "Buffer empty? " << (buffer.empty() ? "Yes" : "No") << std::endl;
            std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;

            buffer.push(2);
            buffer.push(3);
            buffer.push(4);
            buffer.push(5);
            std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;

            buffer.push(6);

            std::cout << "Buffer size: " << buffer.size() << std::endl;
            std::cout << "Buffer capacity: " << buffer.capacity() << std::endl;
            std::cout << "Buffer full? " << (buffer.full() ? "Yes" : "No") << std::endl;
            std::cout << "Buffer empty? " << (buffer.empty() ? "Yes" : "No") << std::endl;
            buffer.print(); // This will print the contents of the buffer, which should be "1 2 3".

            int poppedData = buffer.pop();
            std::cout << "Popped data: " << poppedData << std::endl;
            buffer.print();

        }
    }
}