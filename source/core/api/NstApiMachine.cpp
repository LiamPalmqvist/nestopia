////////////////////////////////////////////////////////////////////////////////////////
//
// Nestopia - NES/Famicom emulator written in C++
//
// Copyright (C) 2003-2008 Martin Freij
//
// This file is part of Nestopia.
//
// Nestopia is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Nestopia is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Nestopia; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////////////

#include <new>
#include "../NstMachine.hpp"
#include "../NstImage.hpp"
#include "../NstState.hpp"
#include "NstApiMachine.hpp"

#include <iostream>

namespace Nes
{
	namespace Api
	{
		Machine::EventCaller Machine::eventCallback;

		uint Machine::Is(uint a) const throw()
		{
			return emulator.Is( a );
		}

		bool Machine::Is(uint a,uint b) const throw()
		{
			return emulator.Is( a, b );
		}

		bool Machine::IsLocked() const
		{
			return emulator.tracker.IsLocked();
		}

		#ifdef NST_MSVC_OPTIMIZE
		#pragma optimize("s", on)
		#endif

		Result Machine::Load(std::istream& stream,FavoredSystem system,AskProfile ask,Patch* patch,uint type)
		{
			Result result;

			const bool on = Is(ON);

			try
			{
				result = emulator.Load
				(
					stream,
					static_cast<Core::FavoredSystem>(system),
					ask == ASK_PROFILE,
					patch ? &patch->stream : NULL,
					patch ? patch->bypassChecksum : false,
					patch ? &patch->result : NULL,
					type
				);
			}
			catch (Result r)
			{
				return r;
			}
			catch (const std::bad_alloc&)
			{
				return RESULT_ERR_OUT_OF_MEMORY;
			}
			catch (...)
			{
				return RESULT_ERR_GENERIC;
			}

			if (on)
				Power( true );

			return result;
		}

		Result Machine::Load(std::istream& stream,FavoredSystem system,AskProfile ask) throw()
		{
			return Load( stream, system, ask, NULL, Core::Image::UNKNOWN );
		}

		Result Machine::Load(std::istream& stream,FavoredSystem system,Patch& patch,AskProfile ask) throw()
		{
			return Load( stream, system, ask, &patch, Core::Image::UNKNOWN );
		}

		Result Machine::LoadCartridge(std::istream& stream,FavoredSystem system,AskProfile ask) throw()
		{
			return Load( stream, system, ask, NULL, Core::Image::CARTRIDGE );
		}

		Result Machine::LoadCartridge(std::istream& stream,FavoredSystem system,Patch& patch,AskProfile ask) throw()
		{
			return Load( stream, system, ask, &patch, Core::Image::CARTRIDGE );
		}

		Result Machine::LoadDisk(std::istream& stream,FavoredSystem system) throw()
		{
			return Load( stream, system, DONT_ASK_PROFILE, NULL, Core::Image::DISK );
		}

		Result Machine::LoadSound(std::istream& stream,FavoredSystem system) throw()
		{
			return Load( stream, system, DONT_ASK_PROFILE, NULL, Core::Image::SOUND );
		}

		Result Machine::Unload() throw()
		{
			if (!Is(IMAGE))
				return RESULT_NOP;

			return emulator.Unload();
		}

		// This is the part of the emulator which actually starts the emulation
		Result Machine::Power(const bool on) throw()
		{
			// If the machine is already in the desired power state, do nothing
			if (on == bool(Is(ON)))
				return RESULT_NOP;

			// If we're trying to power on, but the machine isn't ready, return an error
			if (on)
			{
				try
				{
					// If the machine is already on, we want to reset it instead of powering it on again.
					// This is because some parts of the emulator (such as the PPU) need to be reset when the machine is
					// powered on, and if we just call Power( true ) when the machine is already on, those parts won't
					// be reset. By calling Reset( true ) instead, we ensure that the machine is
					// properly reset when it's already on.
					emulator.Reset( true );
				}
				catch (Result result)
				{
					return result;
				}
				catch (const std::bad_alloc&)
				{
					return RESULT_ERR_OUT_OF_MEMORY;
				}
				catch (...)
				{
					return RESULT_ERR_GENERIC;
				}

				// If we successfully powered on the machine, return OK
				return RESULT_OK;
			}
			else
			{
				return emulator.PowerOff();
			}
		}

		Result Machine::Reset(const bool hard) throw()
		{
			// If the machine isn't on, or if it's locked, we can't reset it, so return an error
			if (!Is(ON) || IsLocked())
				return RESULT_ERR_NOT_READY;

			try
			{
				emulator.Reset( hard );
			}
			catch (Result result)
			{
				return result;
			}
			catch (const std::bad_alloc&)
			{
				return RESULT_ERR_OUT_OF_MEMORY;
			}
			catch (...)
			{
				return RESULT_ERR_GENERIC;
			}

			return RESULT_OK;
		}

		Result Machine::SetRamPowerState(const uint state) throw()
		{
			emulator.SetRamPowerState(state);
			return RESULT_OK;
		}

		Machine::Mode Machine::GetMode() const throw()
		{
			return static_cast<Mode>(Is(NTSC|PAL));
		}

		Machine::Mode Machine::GetDesiredMode() const throw()
		{
			return (!emulator.image || emulator.image->GetDesiredRegion() == Core::REGION_NTSC) ? NTSC : PAL;
		}

		Result Machine::SetMode(const Mode mode) throw()
		{
			if (mode == GetMode())
				return RESULT_NOP;

			Result result = Power( false );

			if (NES_SUCCEEDED(result))
			{
				emulator.SwitchMode();

				if (result != RESULT_NOP)
					result = Power( true );
			}

			return result;
		}

		// TODO: This function loads a state from stream, saving this for later
		Result Machine::LoadState(std::istream& stream) throw()
		{
			if (!Is(GAME,ON) || IsLocked())
				return RESULT_ERR_NOT_READY;

			try
			{
				emulator.tracker.Resync();
				Core::State::Loader loader( &stream, true );

				if (emulator.LoadState( loader, true ))
					return RESULT_OK;
				else
					return RESULT_ERR_INVALID_CRC;
			}
			catch (Result result)
			{
				return result;
			}
			catch (const std::bad_alloc&)
			{
				return RESULT_ERR_OUT_OF_MEMORY;
			}
			catch (...)
			{
				return RESULT_ERR_GENERIC;
			}
		}

		// TODO: This function saves a state to stream, saving this for later
		Result Machine::SaveState(std::ostream& stream,Compression compression) const throw()
		{
			if (!Is(GAME,ON))
				return RESULT_ERR_NOT_READY;

			try
			{
				Core::State::Saver saver( &stream, compression != NO_COMPRESSION, false );
				emulator.SaveState( saver );
			}
			catch (Result result)
			{
				return result;
			}
			catch (const std::bad_alloc&)
			{
				return RESULT_ERR_OUT_OF_MEMORY;
			}
			catch (...)
			{
				return RESULT_ERR_GENERIC;
			}

			return RESULT_OK;
		}

		#ifdef NST_MSVC_OPTIMIZE
		#pragma optimize("", on)
		#endif
	}
}
