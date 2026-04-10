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

#include "NstState.hpp"
#include "NstZlib.hpp"

namespace Nes
{
	namespace Core
	{
		namespace State
		{
			enum Compression
			{
				NO_COMPRESSION,
				ZLIB_COMPRESSION
			};

			#ifdef NST_MSVC_OPTIMIZE
			#pragma optimize("s", on)
			#endif

			/*!
			 * The Saver class is responsible for writing state data to a stream.
			 * The stream is a file which can be loaded by the `Loader` Class
			 * @param p - The stream to write to
			 * @param c - Whether to use compression when writing data
			 * @param i - Whether the state being saved is an internal state (as opposed to a save state)
			 * @param append - If non-zero, the offset in the stream to append the state data to (used for internal states)
			 */
			Saver::Saver(StdStream p, bool c, bool i, dword append)
			: stream(p), chunks(CHUNK_RESERVE), useCompression(c), internal(i)
			{
				NST_COMPILE_ASSERT( CHUNK_RESERVE >= 2 );

				chunks.SetTo(1);
				chunks.Front() = 0;

				if (append)
				{
					chunks.SetTo(2);
					chunks[1] = append;
					stream.Seek( 4 + 4 + append );
				}
			}

			/*!
			 * The destructor of the Saver class verifies that all chunks have been properly closed (i.e. that there is only one chunk left in the stack, which is the initial chunk)
			 */
			Saver::~Saver()
			{
				NST_VERIFY( chunks.Size() == 1 );
			}

			#ifdef NST_MSVC_OPTIMIZE
			#pragma optimize("", on)
			#endif

			/*!
			 * Begins a new chunk of state data. A chunk is a section of the state file which contains related data, and is identified by a 32-bit integer (the `chunk` parameter). The chunk is written to the stream with a placeholder for its length, which is updated when the chunk is ended. Chunks can be nested, allowing for hierarchical organization of state data.
			 * @param chunk A 32-bit integer that identifies the type of data being written in the chunk. This can be used to differentiate between different types of state data when loading the state later on.
			 * @return
			 */
			Saver& Saver::Begin(dword chunk)
			{
				stream.Write32( chunk );
				stream.Write32( 0 );
				chunks.Append( 0 );

				return *this;
			}

			/* At this point a chunk has just been fully finalised and its length patched back into the stream.
			 * if chunks.Size() == 1 after Pop(), this is the top-level chunk closing - i.e. the complete state
			 * has just been written.
			 */
			Saver& Saver::End()
			{
				NST_ASSERT( chunks.Size() > 1 );

				const dword written = chunks.Pop();
				chunks.Back() += 4 + 4 + written;

				stream.Seek( -idword(written + 4) );
				stream.Write32( written );
				stream.Seek( written );

				// ----------------------------------------------------------------
				// INJECT CODE HERE TO RETRIEVE STATE FROM EMULATOR
				// ----------------------------------------------------------------

				return *this;
			}

			Saver& Saver::Write8(uint data)
			{
				chunks.Back() += 1;
				stream.Write8( data );
				return *this;
			}

			Saver& Saver::Write16(uint data)
			{
				chunks.Back() += 2;
				stream.Write16( data );
				return *this;
			}

			Saver& Saver::Write32(dword data)
			{
				chunks.Back() += 4;
				stream.Write32( data );
				return *this;
			}

			Saver& Saver::Write64(qaword data)
			{
				chunks.Back() += 8;
				stream.Write64( data );
				return *this;
			}

			Saver& Saver::Write(const byte* data,dword length)
			{
				chunks.Back() += length;
				stream.Write( data, length );
				return *this;
			}



			Saver& Saver::Compress(const byte* const data,const dword length)
			{
				NST_VERIFY( length );

				if (Zlib::AVAILABLE && useCompression && length > 1)
				{
					Vector<byte> buffer( length - 1 );

					if (const dword compressed = Zlib::Compress( data, length, buffer.Begin(), buffer.Size(), Zlib::BEST_COMPRESSION ))
					{
						chunks.Back() += 1 + compressed;
						stream.Write8( ZLIB_COMPRESSION );
						stream.Write( buffer.Begin(), compressed );
						return *this;
					}
				}

				chunks.Back() += 1 + length;
				stream.Write8( NO_COMPRESSION );
				stream.Write( data, length );

				return *this;
			}

			#ifdef NST_MSVC_OPTIMIZE
			#pragma optimize("s", on)
			#endif

			/*!
			 * The Loader class is responsible for reading state data from a stream.
			 * The stream is a file which is supposed to have been saved by the `Saver` Class
			 * @param p - The stream to read from
			 * @param c - Whether the data being read was compressed or not (this should match the value of `useCompression` used when saving the state)
			 */
			Loader::Loader(StdStream p,bool c)
			: stream(p), chunks(CHUNK_RESERVE), checkCrc(c)
			{
				chunks.SetTo(0);
			}

			Loader::~Loader()
			{
				NST_VERIFY( chunks.Size() <= 1 );
			}

			#ifdef NST_MSVC_OPTIMIZE
			#pragma optimize("", on)
			#endif

			dword Loader::Begin()
			{
				if (chunks.Size() && !chunks.Back())
					return 0;

				const dword chunk = stream.Read32();
				const dword length = stream.Read32();

				if (chunks.Size())
				{
					if (chunks.Back() >= 4+4+length)
						chunks.Back() -= 4+4+length;
					else
						throw RESULT_ERR_CORRUPT_FILE;
				}

				chunks.Append( length );

				return chunk;
			}

			dword Loader::Length() const
			{
				return chunks.Size() ? chunks.Back() : 0;
			}

			dword Loader::Check()
			{
				return chunks.Size() && !chunks.Back() ? 0 : stream.Peek32();
			}

			void Loader::End()
			{
				if (const dword remaining = chunks.Pop())
				{
					NST_DEBUG_MSG("unreferenced state chunk data!");
					stream.Seek( remaining );
				}
			}

			void Loader::End(dword rollBack)
			{
				if (const idword back = -idword(rollBack+4+4) + idword(chunks.Pop()))
					stream.Seek( back );
			}

			void Loader::CheckRead(dword length)
			{
				if (chunks.Back() >= length)
					chunks.Back() -= length;
				else
					throw RESULT_ERR_CORRUPT_FILE;
			}

			uint Loader::Read8()
			{
				CheckRead( 1 );
				return stream.Read8();
			}

			uint Loader::Read16()
			{
				CheckRead( 2 );
				return stream.Read16();
			}

			dword Loader::Read32()
			{
				CheckRead( 4 );
				return stream.Read32();
			}

			qaword Loader::Read64()
			{
				CheckRead( 8 );
				return stream.Read64();
			}

			void Loader::Read(byte* const data,const dword length)
			{
				CheckRead( length );
				stream.Read( data, length );
			}

			void Loader::Uncompress(byte* const data,const dword length)
			{
				NST_VERIFY( length );

				switch (Read8())
				{
					case NO_COMPRESSION:

						Read( data, length );
						break;

					case ZLIB_COMPRESSION:

						if (!Zlib::AVAILABLE)
						{
							throw RESULT_ERR_UNSUPPORTED;
						}
						else if (chunks.Back())
						{
							Vector<byte> buffer( chunks.Back() );
							Read( buffer.Begin(), buffer.Size() );

							if (Zlib::Uncompress( buffer.Begin(), buffer.Size(), data, length ))
								break;
						}
						// fallthrough

					default:

						throw RESULT_ERR_CORRUPT_FILE;
				}
			}
		}
	}
}
