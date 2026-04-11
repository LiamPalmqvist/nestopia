//
// Created by Liam on 10/04/2026.
//

#ifndef NESTOPIA_REWINDMANAGER_HPP
#define NESTOPIA_REWINDMANAGER_HPP
#include <iostream>

#include "RingBuffer.hpp"
#include "../core/board/NstBoard.hpp"
namespace Nes
{
    namespace Custom
    {
        class RewindManager
        {
            RingBuffer<int> _buffer; // We need this to hold a minute of rewinds
            Core::Boards::Board::Event _event; // This should hopefully provide the timing for the frame_end
            int frame = 0;

            public:
                RewindManager();
        };
    } // Custom
} // Nes

#endif //NESTOPIA_REWINDMANAGER_HPP