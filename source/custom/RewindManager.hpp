//
// Created by Liam on 10/04/2026.
//

#ifndef NESTOPIA_REWINDMANAGER_HPP
#define NESTOPIA_REWINDMANAGER_HPP
#include <iostream>

#include "RingBuffer.hpp"
#include "../core/api/NstApiMachine.hpp"
#include "DataDiff.hpp"

namespace Nes
{
    namespace Custom
    {
        class RewindManager
        {
            // wtf is this code man. A `RingBuffer<>` of `std::vector<>` of `DataDiff<char>` which holds differences?? I need to make this more simple
            RingBuffer<std::vector<DataDiff<char>::difference>> _buffer; // We need this to hold a minute of rewinds
            std::vector<char> _currentState; // The size of a save state is 5061 bytes, so we need to make sure that the current state is always the correct size. This will hold the current state of the machine, which we can compare to the new state to get the differences.;
            std::vector<char> _previousState;
            bool _rewinding = false;

            // a difference comprises two variables, the byte index and the difference
            // the differences are stored in a vector as a list of differences
            // then the list of differences are stored in the ring buffer
            // holy shit this is jank

        public:
            // The initialiser creates a buffer of size 3600, which should be enough to hold a minute of savestates at 60fps. We can also allow the user to specify a custom buffer size if they want to.
            explicit RewindManager() : _buffer(3600) {}
            explicit RewindManager(int bufferSize) : _buffer(bufferSize) {}

            // We don't want to allow copying of the RewindManager, as it would cause issues with the buffer.
            RewindManager(RewindManager& other) = delete;
            RewindManager& operator=(const RewindManager&) = delete;

            static RewindManager& getInstance(int bufferSize = 3600);

            static void resizeBuffer(int newSize) { getInstance()._buffer.resize(newSize); }

            static void clearBuffer() { getInstance()._buffer.clearBuffer(); }

            void setRewinding(bool rewinding) { _rewinding = rewinding; }
            bool isRewinding() { return _rewinding; }

            void capture(Api::Machine* machine);
            void rewind(Api::Machine* machine);
        };
    } // Custom
} // Nes

#endif //NESTOPIA_REWINDMANAGER_HPP