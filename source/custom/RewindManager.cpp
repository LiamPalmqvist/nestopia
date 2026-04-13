//
// Created by Liam on 10/04/2026.
//
#include "RewindManager.hpp"
#include "core/NstStream.hpp"
#include <sstream>

namespace Nes
{
    namespace Custom
    {
        void RewindManager::capture(Api::Machine* machine)
        {
            // if (Nes::Api::Machine::ON)
            // {
            //     std::cerr << "RewindManager::capture() - machine pointer is null!" << std::endl;
            //     return;
            // }

            // open a memory stream to write the savestate data to
            // This is a little dangerous as it exposes the memory directly in the program
            std::ostringstream oss;

            machine->SaveState(oss, Nes::Api::Machine::NO_COMPRESSION);

            std::string currentStateStr = oss.str();

            // Convert the savestate data from the string stream to a vector of chars, which is what the DataDiff class expects. This is a bit of a hack, but it works for our purposes.
            _currentState = std::vector(currentStateStr.begin(), currentStateStr.end());
            // Now we have the savestate data in the string stream

            if (_buffer.empty() && _previousState.empty())
            {
                _previousState = _currentState;
            } else
            {
                // This gives us the differences between the two states
                _buffer.push(DataDiff<char>::CompareFiles(_previousState, _currentState));
                // We need to update the previous state to be the current state for the next capture
                _previousState = _currentState;
            }

            // Hypothetically, this is all we need to keep track of the differences between states.
            std::cout << "Captured state. Buffer size: " << _buffer.size() << std::endl;
            //std::cout << "State contents: " << std::string(_currentState.begin(), _currentState.end()) << std::endl;
        }

        void RewindManager::rewind(Api::Machine* machine)
        {
            // wait since we have the previous state, we can just load that into the machine and update the current state to be that
            // and then apply the changes from the buffer to the previous state
            // effectively pre-loading the changes

            // Step 1, check if buffer is empty - if it isn't, we can rewind
            // Step 2, if it is, we can't rewind, so just return
            // Step 3, if we can rewind, we need to get the most recent state from the buffer and apply it to the current state
            // Step 4, we need to update the current state to be the previous state
            // Step 5, We need to convert the current state from a vector of chars back to a string stream, which is what the machine expects for loading a savestate
            // Step 6, we need to load the new current state into the machine

            // Step 1
            if (_buffer.empty())
            {
                // Step 2
                return;
            }

            // Step 3
            // Grab the differences from the buffer
            const std::vector<DataDiff<char>::difference> currentDifferences = _buffer.pop();

            // Step 4
            // apply the differences to the current state to get the previous state
            _currentState = DataDiff<char>::applyDifferences(_currentState, currentDifferences);

            // Step 5
            // We need to convert the current state from a vector of chars back to a string stream, which is what the machine expects for loading a savestate
            std::string currentStateStr(_currentState.begin(), _currentState.end());
            std::istringstream iss(currentStateStr);

            machine->LoadState(iss);

            std::cout << "Rewound state. Buffer size: " << _buffer.size() << std::endl;
        }

        RewindManager& RewindManager::getInstance(int bufferSize)
        {
            static RewindManager instance(bufferSize);
            return instance;
        }
    } // Custom
} // Nes