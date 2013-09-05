#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDL.h>
#include <math.h>

#if defined(NDEBUG) && !defined(_DEBUG) && !defined(DEBUG)
#	define DumpInstruction( _Message, ... )	do { } while ( 0 )
#else
#	define DumpInstruction( _Message, ... )	do { printf( _Message "\n", __VA_ARGS__ ); } while ( 0 )
#endif

typedef Uint16 address;
typedef Uint16 instruction;

struct Chip8
{
	Chip8( )
	{
		Memory[  0 ] = 0xF0; Memory[  1 ] = 0x90; Memory[  2 ] = 0x90; Memory[  3 ] = 0x90; Memory[  4 ] = 0xF0;	// 0
		Memory[  5 ] = 0x20; Memory[  6 ] = 0x60; Memory[  7 ] = 0x20; Memory[  8 ] = 0x20; Memory[  9 ] = 0x70;	// 1
		Memory[ 10 ] = 0xF0; Memory[ 11 ] = 0x10; Memory[ 12 ] = 0xF0; Memory[ 13 ] = 0x80; Memory[ 14 ] = 0xF0;	// 2
		Memory[ 15 ] = 0xF0; Memory[ 16 ] = 0x10; Memory[ 17 ] = 0xF0; Memory[ 18 ] = 0x10; Memory[ 19 ] = 0xF0;	// 3
		Memory[ 20 ] = 0x90; Memory[ 21 ] = 0x90; Memory[ 22 ] = 0xF0; Memory[ 23 ] = 0x10; Memory[ 24 ] = 0x10;	// 4
		Memory[ 25 ] = 0xF0; Memory[ 26 ] = 0x80; Memory[ 27 ] = 0xF0; Memory[ 28 ] = 0x10; Memory[ 29 ] = 0xF0;	// 5
		Memory[ 30 ] = 0xF0; Memory[ 31 ] = 0x80; Memory[ 32 ] = 0xF0; Memory[ 33 ] = 0x90; Memory[ 34 ] = 0xF0;	// 6
		Memory[ 35 ] = 0xF0; Memory[ 36 ] = 0x10; Memory[ 37 ] = 0x20; Memory[ 38 ] = 0x40; Memory[ 39 ] = 0x40;	// 7
		Memory[ 40 ] = 0xF0; Memory[ 41 ] = 0x90; Memory[ 42 ] = 0xF0; Memory[ 43 ] = 0x90; Memory[ 44 ] = 0xF0;	// 8
		Memory[ 45 ] = 0xF0; Memory[ 46 ] = 0x90; Memory[ 47 ] = 0xF0; Memory[ 48 ] = 0x10; Memory[ 49 ] = 0xF0;	// 9
		Memory[ 50 ] = 0xF0; Memory[ 51 ] = 0x90; Memory[ 52 ] = 0xF0; Memory[ 53 ] = 0x90; Memory[ 54 ] = 0x90;	// a
		Memory[ 55 ] = 0xE0; Memory[ 56 ] = 0x90; Memory[ 57 ] = 0xE0; Memory[ 58 ] = 0x90; Memory[ 59 ] = 0xE0;	// b
		Memory[ 60 ] = 0xF0; Memory[ 61 ] = 0x80; Memory[ 62 ] = 0x80; Memory[ 63 ] = 0x80; Memory[ 64 ] = 0xF0;	// c
		Memory[ 65 ] = 0xE0; Memory[ 66 ] = 0x90; Memory[ 67 ] = 0x90; Memory[ 68 ] = 0x90; Memory[ 69 ] = 0xE0;	// d
		Memory[ 70 ] = 0xF0; Memory[ 71 ] = 0x80; Memory[ 72 ] = 0xF0; Memory[ 73 ] = 0x80; Memory[ 74 ] = 0xF0;	// e
		Memory[ 75 ] = 0xF0; Memory[ 76 ] = 0x80; Memory[ 77 ] = 0xF0; Memory[ 78 ] = 0x80; Memory[ 79 ] = 0x80;	// f
	}

	struct CommandProcessingUnit
	{
		CommandProcessingUnit( ) : StackLevel( 0 )
		{ 
			for ( size_t ix = 0; ix < 16; ++ix )
			{
				Stack[ ix ] = 0;
			}
		}

		struct Registers
		{
			Registers( )
			{
				for ( size_t ix = 0; ix < 16; ++ix )
				{
					v[ ix ] = 0;
				}
				i     = 0;
				delay = 0;
				sound = 0;
				pc    = 0x200;
			}
			unsigned __int8  v[ 16 ];
			unsigned __int16 i;
			unsigned __int8  delay;
			unsigned __int8  sound;
			unsigned __int16 pc;
		};

		Registers			Regs;
		unsigned __int16	Stack[ 16 ];
		size_t				StackLevel;
	};

	CommandProcessingUnit Cpu;

	Uint8 Memory[ 16 * 1024 ];
};


class Api
{
public:

	Api( ) : m_pScreen( NULL ), m_pPixels( NULL ), m_SoundOn( false )
	{
		memset( m_Keys, 0, sizeof( bool ) * 16 );
	}

	void Initialise( )
	{
		//Start SDL
		SDL_Init( SDL_INIT_EVERYTHING );

		//Set up screen
		m_pScreen = SDL_SetVideoMode( 640, 320, 32, SDL_SWSURFACE );

		// Draw stuff.
		if( SDL_MUSTLOCK( m_pScreen ) )
		{
			// Lock the surface
			SDL_LockSurface( m_pScreen );
		}

		m_pPixels = ( Uint32 * )m_pScreen->pixels;

		// Audio
		SDL_AudioSpec desiredSpec;

		desiredSpec.freq = 44100;
		desiredSpec.format = AUDIO_S16SYS;
		desiredSpec.channels = 1;
		desiredSpec.samples = 512;
		desiredSpec.callback = AudioFn;
		desiredSpec.userdata = this;

		SDL_AudioSpec obtainedSpec;

		// you might want to look for errors here
		SDL_OpenAudio(&desiredSpec, &obtainedSpec);

		// start play audio
		SDL_PauseAudio(0);
	}

	bool DrawAt( unsigned __int8 x, unsigned __int8 y, unsigned __int8 h, unsigned __int8 * memory )
	{
		bool anyPixelsUnset = false;

		for ( size_t iy = 0; iy < h; ++iy )
		{
			unsigned __int8 & rRowData = memory[ iy ];

			for ( size_t ix = 0; ix < 8; ++ix )
			{
				if ( rRowData & ( 1 << ( ( 7 - ix ) ) ) )
				{
					anyPixelsUnset |= FlipPixel( x + ix, y + iy );
				}
			}
		}

		return anyPixelsUnset;
	}

	void Tick( )
	{
		if( SDL_MUSTLOCK( m_pScreen ) )
		{
			// Lock the surface
			SDL_UnlockSurface( m_pScreen );
		}

		// Update Screen.
		SDL_Flip( m_pScreen );

		// Draw stuff.
		if( SDL_MUSTLOCK( m_pScreen ) )
		{
			// Lock the surface
			SDL_LockSurface( m_pScreen );
		}

		m_pPixels = ( Uint32 * )m_pScreen->pixels;

		// Input.
		SDL_Event e;
		while ( SDL_PollEvent( &e ) )
		{
			if ( e.type == SDL_KEYDOWN )
			{
				unsigned __int8 index = SDLKeyToInputIndex( e.key.keysym.sym );
				if ( index != 0xff )
				{
					m_Keys[ index ] = true;
				}
			}
			else if ( e.type == SDL_KEYUP )
			{
				unsigned __int8 index = SDLKeyToInputIndex( e.key.keysym.sym );
				if ( index != 0xff )
				{
					m_Keys[ index ] = false;
				}
			}
		}
	}

	void ClearScreen( )
	{
		if( SDL_MUSTLOCK( m_pScreen ) )
		{
			// Lock the surface
			SDL_UnlockSurface( m_pScreen );
		}

		// Clear screen.
		SDL_FillRect( m_pScreen, NULL, 0x000000 );

		// Draw stuff.
		if( SDL_MUSTLOCK( m_pScreen ) )
		{
			// Lock the surface
			SDL_LockSurface( m_pScreen );
		}

		m_pPixels = ( Uint32 * )m_pScreen->pixels;
	}

	bool IsKeyDown( Uint8 ix )
	{
		if ( ix >= 0 && ix <= 16 )
			return m_Keys[ ix ];

		return false;
	}

	void SetSound( bool on )
	{
		m_SoundOn = on;
	}

	void Destroy( )
	{
		//Quit SDL
		SDL_Quit( );
	}

private:

	bool FlipPixel( unsigned __int8 x, unsigned __int8 y )
	{
		if ( x < 0 || x >= 64 || y < 0 || y >= 32 )
			return false;

		bool anyPixelsUnset = false;

		for ( size_t iy = 0; iy < 10; ++iy )											
		{																				
			for ( size_t ix = 0; ix < 10; ++ix )											
			{		
				Uint32 & rPixel = m_pPixels[ ( y * 10 + iy ) * ( m_pScreen->pitch / 4 ) + x * 10 + ix ];
				if ( rPixel )
				{
					anyPixelsUnset = true;
					rPixel = 0;
				}
				else
				{
					rPixel = 0xffffff;	
				}
			}																				
		}	

		return anyPixelsUnset;
	}

	unsigned __int8 SDLKeyToInputIndex( SDLKey & key )
	{
		switch ( key )
		{
			case SDLK_1:
			{
				return 1;
			}
			break;
			case SDLK_2:
			{
				return 2;
			}
			break;
			case SDLK_3:
			{
				return 3;
			}
			break;
			case SDLK_4:
			{
				return 0xc;
			}
			break;

			case SDLK_q:
			{
				return 4;
			}
			break;
			case SDLK_w:
			{
				return 5;
			}
			break;
			case SDLK_e:
			{
				return 6;
			}
			break;
			case SDLK_r:
			{
				return 0xd;
			}
			break;

			case SDLK_a:
			{
				return 7;
			}
			break;
			case SDLK_s:
			{
				return 8;
			}
			break;
			case SDLK_d:
			{
				return 9;
			}
			break;
			case SDLK_f:
			{
				return 0xe;
			}
			break;

			case SDLK_z:
			{
				return 0xa;
			}
			break;
			case SDLK_x:
			{
				return 0;
			}
			break;
			case SDLK_c:
			{
				return 0xb;
			}
			break;
			case SDLK_v:
			{
				return 0xf;
			}
			break;
		}
		return 0xff;
	}

	static void AudioFn( void * userData, Uint8 * stream, int length )
	{
		Api * thisPtr = ( Api *)userData;

		if ( thisPtr->m_SoundOn )
		{
			int i = 0;
			while ( i < length ) 
			{
				stream[i] = ( Uint8 )( 28000 * sinf( i * 0.5f * 2.f * 3.1415f / 44100.f ) );
				i++;
			}
		}
		else
		{
			memset( stream, 0, length );
		}
	}

	SDL_Surface * m_pScreen;
	Uint32 * m_pPixels;
	bool m_Keys[ 16 ];
	bool m_SoundOn;
};


int main( int numArgs, char ** args )
{
	Chip8 chip8;
	Api api;
	api.Initialise( );

	FILE * fh = NULL;
	if ( fopen_s( &fh, "Pong.ch8", "rb" ) == 0 )
	{
		fseek( fh, 0L, SEEK_END );
		size_t fileSize = ftell( fh );
		fseek( fh, 0L, SEEK_SET );
		assert( sizeof( chip8.Memory ) - 0x200 >= fileSize );
		int numRead = fread( &chip8.Memory[ 0x200 ], fileSize, 1, fh );
		assert( numRead == 1 );
		fclose( fh );
	}
	else
	{
		assert( 0 );
		return -1;
	}

	// Previous instruction (for debugging).
	address lastInstruction = 0x0000;

	// Last time we did our 60Hz update.
	Uint32 last60HzTime = SDL_GetTicks( );

	// Instructions processed since last 60hz interval.
	Uint8 instructionsProcessedSinceLastUpdate = 0;

	// Loop forever.
	for ( ; ; )
	{
		// Get time (in milliseconds).
		Uint32 timeNow = SDL_GetTicks( );

		if ( instructionsProcessedSinceLastUpdate >= 15 )
		{
			while ( timeNow - last60HzTime <= 1000.f / 60.f )
			{
				SDL_Delay( 0 );
				timeNow = SDL_GetTicks( );
			}

			instructionsProcessedSinceLastUpdate = 0;
		}

		// If it has been 60Hz since our last update...
		if ( timeNow - last60HzTime > 1000.f / 60.f )
		{
			// Decrement delay register.
			if ( chip8.Cpu.Regs.delay > 0 )
				chip8.Cpu.Regs.delay--;

			// Decrement sound register.
			if ( chip8.Cpu.Regs.sound > 0 )
				chip8.Cpu.Regs.sound--;

			// Update to know when next 60Hz timer should be issued.
			last60HzTime = timeNow;

			// Update API (render to screen, process keys, etc.)
			api.Tick( );
		}

		// Play bleeping sound.
		api.SetSound( chip8.Cpu.Regs.sound > 0 );

		Uint16 instruction = ( ( ( address )chip8.Memory[ chip8.Cpu.Regs.pc ] ) << 8 ) | ( address )chip8.Memory[ chip8.Cpu.Regs.pc + 1 ];

		switch ( instruction )
		{
		case 0x00e0:
			DumpInstruction( "Screen clear" );
			api.ClearScreen( );
			break;

		case 0x00ee:
			DumpInstruction( "Return from sub routine" );

			// Pop current PC from stack.
			assert( chip8.Cpu.StackLevel );
			chip8.Cpu.Regs.pc = chip8.Cpu.Stack[ --chip8.Cpu.StackLevel ];

			break;

		default:
			switch ( instruction & 0xf000 )
			{
			case 0x0000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Calls RCA 1802 program at address 0x%x", addr );

					// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#0nnn
					// This instruction is only used on the old computers on which Chip-8 was originally implemented. It is ignored by modern interpreters.

				}
				break;
			case 0x1000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Jump to 0x%x", addr );

					// Set PC, we do -2 as PC is incremented by two after each instruction.
					chip8.Cpu.Regs.pc = addr - 2;
				}
				break;
			case 0x2000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Call sub routine to 0x%x", addr );

					// Store current PC on stack.
					chip8.Cpu.Stack[ chip8.Cpu.StackLevel++ ] = chip8.Cpu.Regs.pc;
					assert( chip8.Cpu.StackLevel < 16 );

					// Set PC, we do -2 as PC is incremented by two after each instruction.
					chip8.Cpu.Regs.pc = addr - 2;
				}
				break;
			case 0x3000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Skips next instruction if v%02d equals %d", reg, val );

					if ( chip8.Cpu.Regs.v[ reg ] == val )
					{
						chip8.Cpu.Regs.pc += 2;
					}
				}
				break;
			case 0x4000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Skips next instruction if v%02d does not equals %d", reg, val );

					if ( chip8.Cpu.Regs.v[ reg ] != val )
					{
						chip8.Cpu.Regs.pc += 2;
					}
				}
				break;
			case 0x5000:
				{
					int reg1 = ( instruction & 0x0f00 ) >> 8;
					int reg2 = ( instruction & 0x00f0 ) >> 4;
					DumpInstruction( "Skips next instruction if v%02d equals v%02d", reg1, reg2 );

					if ( chip8.Cpu.Regs.v[ reg1 ] == chip8.Cpu.Regs.v[ reg2 ] )
					{
						chip8.Cpu.Regs.pc += 2;
					}
				}
				break;
			case 0x6000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Set v%02d to %d", reg, val );

					chip8.Cpu.Regs.v[ reg ] = val;
				}
				break;
			case 0x7000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Add %d to v%02d", val, reg );

					chip8.Cpu.Regs.v[ reg ] += val;
				}
				break;
			case 0x8000:
				{
					int reg1 = ( instruction & 0x0f00 ) >> 8;
					int reg2 = ( instruction & 0x00f0 ) >> 4;

					switch ( instruction & 0x000f )
					{
					case 0:
						DumpInstruction( "Sets v%02d to v%02d", reg1, reg2 );
						chip8.Cpu.Regs.v[ reg1 ] = chip8.Cpu.Regs.v[ reg2 ];
						break;
					case 1:
						DumpInstruction( "Sets v%02d to v%02d OR v%02d", reg1, reg1, reg2 );
						chip8.Cpu.Regs.v[ reg1 ] = chip8.Cpu.Regs.v[ reg1 ] | chip8.Cpu.Regs.v[ reg2 ];
						break;
					case 2:
						DumpInstruction( "Sets v%02d to v%02d AND v%02d", reg1, reg1, reg2 );
						chip8.Cpu.Regs.v[ reg1 ] = chip8.Cpu.Regs.v[ reg1 ] & chip8.Cpu.Regs.v[ reg2 ];
						break;
					case 3:
						DumpInstruction( "Sets v%02d to v%02d XOR v%02d", reg1, reg1, reg2 );
						chip8.Cpu.Regs.v[ reg1 ] = chip8.Cpu.Regs.v[ reg1 ] ^ chip8.Cpu.Regs.v[ reg2 ];
						break;
					case 4:
						{
							DumpInstruction( "Sets v%02d to v%02d + v%02d [VF = 1 when there is a carry, 0 when not]", reg1, reg1, reg2 );
							unsigned __int16 result = chip8.Cpu.Regs.v[ reg1 ] + chip8.Cpu.Regs.v[ reg2 ];
							chip8.Cpu.Regs.v[ reg1 ] = result & 0xff;
							chip8.Cpu.Regs.v[ 0xf ] = ( result > 0xff ) ? 1 : 0;
						}
						break;
					case 5:
						DumpInstruction( "Sets v%02d to v%02d - v%02d [VF = 0 when there is a borrow, 1 when not]", reg1, reg1, reg2 );
						chip8.Cpu.Regs.v[ 0xf ] = ( chip8.Cpu.Regs.v[ reg1 ] >= chip8.Cpu.Regs.v[ reg2 ] ) ? 1 : 0;
						chip8.Cpu.Regs.v[ reg1 ] = chip8.Cpu.Regs.v[ reg1 ] - chip8.Cpu.Regs.v[ reg2 ];
						break;
					case 6:
						DumpInstruction( "Shift v%02d right by one [VF is set to the least significant bit of v%02d before the shift]", reg1, reg1 );
						chip8.Cpu.Regs.v[ 0xf ] = chip8.Cpu.Regs.v[ reg1 ] & 0x1;
						chip8.Cpu.Regs.v[ reg1 ] >>= 1;
						break;
					case 7:
						DumpInstruction( "Sets v%02d to v%02d - v%02d [VF = 0 when there is a borrow, 1 when not]", reg1, reg2, reg1 );
						chip8.Cpu.Regs.v[ 0xf ] = ( chip8.Cpu.Regs.v[ reg2 ] >= chip8.Cpu.Regs.v[ reg1 ] ) ? 1 : 0;
						chip8.Cpu.Regs.v[ reg1 ] = chip8.Cpu.Regs.v[ reg2 ] - chip8.Cpu.Regs.v[ reg1 ];
						break;
					case 0xe:
						DumpInstruction( "Shift v%02d left by one [VF is set to the most significant bit of v%02d before the shift]", reg1, reg1 );
						chip8.Cpu.Regs.v[ 0xf ] = ( chip8.Cpu.Regs.v[ reg1 ] & 0x80 ) ? 1 : 0;
						chip8.Cpu.Regs.v[ reg1 ] <<= 1;
						break;

					default:
						assert( 0 );
					}
				}
				break;
			case 0x9000:
				{
					int reg1 = ( instruction & 0x0f00 ) >> 8;
					int reg2 = ( instruction & 0x00f0 ) >> 4;
					DumpInstruction( "Skips next instruction if v%02d does not equals v%02d", reg1, reg2 );

					if ( chip8.Cpu.Regs.v[ reg1 ] != chip8.Cpu.Regs.v[ reg2 ] )
					{
						chip8.Cpu.Regs.pc += 2;
					}
				}
				break;
			case 0xa000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Set i to 0x%x", addr );

					chip8.Cpu.Regs.i = addr;
				}
				break;
			case 0xb000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Jump to address 0x%x plus v0", addr );

					// Set PC, we do -2 as PC is incremented by two after each instruction.
					chip8.Cpu.Regs.pc = chip8.Cpu.Regs.v[ 0 ] + addr - 2;
				}
				break;
			case 0xc000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Set v%02d to random number and %d", reg, val );

					chip8.Cpu.Regs.v[ reg ] = ( rand( ) % 255 ) & val;
				}
				break;
			case 0xd000:
				{
					int reg1 = ( instruction & 0x0f00 ) >> 8;
					int reg2 = ( instruction & 0x00f0 ) >> 4;
					int val = instruction & 0x000f;
					DumpInstruction( "Draw sprite at v%02d,v%02d width 8, height %d", reg1, reg2, val );

					if ( api.DrawAt( chip8.Cpu.Regs.v[ reg1 ], chip8.Cpu.Regs.v[ reg2 ], val, &chip8.Memory[ chip8.Cpu.Regs.i ] ) )
					{
						chip8.Cpu.Regs.v[ 0xf ] = 1;
					}
					else
					{
						chip8.Cpu.Regs.v[ 0xf ] = 0;
					}

					// Each row drawn from address in i register (a bit rows are 8 bits in i register, MSB on left).
					// If any pixels are turned off from this v[15] is set to 1, otherwise v[15] is set to 0.
				}
				break;
			case 0xe000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;

					switch ( instruction & 0x00ff )
					{
					case 0x9e:
						DumpInstruction( "Skip next instruction if key stored in v%02d is pressed", reg );

						if ( api.IsKeyDown( chip8.Cpu.Regs.v[ reg ] ) )
						{
							chip8.Cpu.Regs.pc += 2;
						}

						break;
					case 0xa1:
						DumpInstruction( "Skip next instruction if key stored in v%02d is not pressed", reg );

						if ( ! api.IsKeyDown( chip8.Cpu.Regs.v[ reg ] ) )
						{
							chip8.Cpu.Regs.pc += 2;
						}

						break;

					default:
						assert( 0 );
					}
				}
				break;
			case 0xf000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;

					switch ( instruction & 0x00ff )
					{
					case 0x7:
						DumpInstruction( "Set v%02d to value of the delay timer", reg );
						chip8.Cpu.Regs.v[ reg ] = chip8.Cpu.Regs.delay;
						break;
					case 0xa:
						DumpInstruction( "Key press awaited and then stored in v%02d", reg );
						assert( 0 );
						break;
					case 0x15:
						DumpInstruction( "Set delay timer to v%02d", reg );
						chip8.Cpu.Regs.delay = chip8.Cpu.Regs.v[ reg ];
						break;
					case 0x18:
						DumpInstruction( "Set sound timer to v%02d", reg );
						chip8.Cpu.Regs.sound = chip8.Cpu.Regs.v[ reg ];
						break;
					case 0x1e:
						DumpInstruction( "Sets i to i + v%02d", reg );
						chip8.Cpu.Regs.i += chip8.Cpu.Regs.v[ reg ];
						break;
					case 0x29:
						// Characters 0-F (in hexadecimal) are represented by a 4x5 font.
						DumpInstruction( "Sets i to the location of the sprite for the character in v%02d", reg );
						chip8.Cpu.Regs.i = chip8.Cpu.Regs.v[ reg ] * 5;
						break;
					case 0x33:
						{
							// With the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2.
							// In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
							DumpInstruction( "Stores Binary-coded decimal representation of v%02d in i", reg );
							int hundreds = chip8.Cpu.Regs.v[ reg ] / 100;
							int tens = ( chip8.Cpu.Regs.v[ reg ] / 10 ) % 10;
							int units = chip8.Cpu.Regs.v[ reg ] % 10;

							chip8.Memory[ chip8.Cpu.Regs.i + 0 ] = hundreds;
							chip8.Memory[ chip8.Cpu.Regs.i + 1 ] = tens;
							chip8.Memory[ chip8.Cpu.Regs.i + 2 ] = units;
						}
						break;
					case 0x55:
						DumpInstruction( "Stores v0 to v%02d in memory starting at i", reg );
						for ( int ix = 0; ix < reg + 1; ++ix )
						{
							chip8.Memory[ chip8.Cpu.Regs.i + ix ] = chip8.Cpu.Regs.v[ ix ];
						}
						break;
					case 0x65:
						DumpInstruction( "Fills v0 to v%02d with memory starting at i", reg );
						for ( int ix = 0; ix < reg + 1; ++ix )
						{
							chip8.Cpu.Regs.v[ ix ] = chip8.Memory[ chip8.Cpu.Regs.i + ix ];
						}
						break;

					default:
						assert( 0 );
					}
				}
				break;

			default:
				assert( 0 );
			}
		}

		// Update last instruction (for debugging).
		lastInstruction = instruction;

		// Jump forward to next instruction.
		chip8.Cpu.Regs.pc += 2;

		// Increment instruction counter.
		instructionsProcessedSinceLastUpdate++;
	}

	api.Destroy( );

	return 0;
}
