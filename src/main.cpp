#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <windows.h>
#include <MMSystem.h>
#include <SDL.h>

#define DumpInstruction( _Message, ... )	\
	printf( _Message "\n", __VA_ARGS__ );

typedef unsigned __int16 address;
typedef unsigned __int16 instruction;

struct Cpu
{
	Cpu( ) : StackLevel( 0 )
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
			i = 0;
			delay = 0;
			sound = 0;
		}
		unsigned __int8 v[ 16 ];
		unsigned __int16 i;
		unsigned __int8 delay;
		unsigned __int8 sound;
	};

	Registers			Regs;
	unsigned __int16	Stack[ 16 ];
	size_t				StackLevel;
};

Cpu g_Cpu;

unsigned __int8 g_Memory[ 16 * 1024 ] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,	// 0
	0x20, 0x60, 0x20, 0x20, 0x70,	// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,	// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,	// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,	// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,	// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,	// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,	// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,	// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,	// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,	// a
	0xE0, 0x90, 0xE0, 0x90, 0xE0,	// b
	0xF0, 0x80, 0x80, 0x80, 0xF0,	// c
	0xE0, 0x90, 0x90, 0x90, 0xE0,	// d
	0xF0, 0x80, 0xF0, 0x80, 0xF0,	// e
	0xF0, 0x80, 0xF0, 0x80, 0x80	// f
};

bool g_Keys[ 16 ];

class Api
{
public:

	Api( ) : m_pScreen( NULL )
	{

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

		// Clear screen.
		// SDL_FillRect( m_pScreen, NULL, 0x000000 );

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
					g_Keys[ index ] = true;
				}
			}
			else if ( e.type == SDL_KEYUP )
			{
				unsigned __int8 index = SDLKeyToInputIndex( e.key.keysym.sym );
				if ( index != 0xff )
				{
					g_Keys[ index ] = false;
				}
			}
		}
	}

	void Destroy( )
	{
		//Quit SDL
		SDL_Quit( );
	}

private:

	bool FlipPixel( unsigned __int8 x, unsigned __int8 y )
	{
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

	SDL_Surface * m_pScreen;
	Uint32 * m_pPixels;
};

Api g_Api;

int main( int numArgs, char ** args )
{
	g_Api.Initialise( );

	instruction data[ ] = {
		0x0800,
		0x00e0,
		0x00ee,
		0x1123,
		0x3480,
		0x4820,
		0x5230,
		0x6209,
		0x7a30,
		0x8120,
		0x8121,
		0x8122,
		0x8123,
		0x8124,
		0x8125,
		0x8126,
		0x8127,
		0x812e,
		0x9230,
		0xa321,
		0xbff0,
		0xc106,
		0xd124,
		0xe19e,
		0xe2a1,
		0xf307,
		0xf40a,
		0xf507,
		0xf60a,
		0xf715,
		0xf818,
		0xf91e,
		0xfa29,
		0xfb33,
		0xfc55,
		0xfd65,
	};
	address pc = 0x200;

	FILE * fh = NULL;
	if ( fopen_s( &fh, "Tetris.ch8", "rb" ) == 0 )
	{
		fseek( fh, 0L, SEEK_END );
		size_t fileSize = ftell( fh );
		fseek( fh, 0L, SEEK_SET );
		assert( sizeof( g_Memory ) - 0x200 >= fileSize );
		int numRead = fread( &g_Memory[ 0x200 ], fileSize, 1, fh );
		assert( numRead == 1 );
		fclose( fh );
	}
	else
	{
		memcpy( &g_Memory[ 0x200 ], data, sizeof( data ) );
	}

	address previousInstruction = 0x0000;

	DWORD last60Hz = timeGetTime( );
	DWORD lastInstruction = 0;

	for ( ; ; )
	{
		DWORD timeNow = timeGetTime( );
		if ( timeNow - last60Hz > 1000.f / 60.f )
		{
			if ( g_Cpu.Regs.delay > 0 )
				g_Cpu.Regs.delay--;

			if ( g_Cpu.Regs.sound > 0 )
				g_Cpu.Regs.sound--;

			last60Hz = timeNow;

			g_Api.Tick( );
		}

		if ( g_Cpu.Regs.sound )
		{
			// Play bleeping sound.
		}

		while ( timeNow - lastInstruction < 1.f / 512 )
		{
			Sleep( 0 );
			timeNow = timeGetTime( );
		}
		lastInstruction = timeNow;

		address instruction = ( ( ( address )g_Memory[ pc ] ) << 8 ) | ( address )g_Memory[ pc + 1 ];

		switch ( instruction )
		{
		case 0x00e0:
			DumpInstruction( "Screen clear" );
			assert( 0 );
			break;

		case 0x00ee:
			DumpInstruction( "Return from sub routine" );

			// Pop current PC from stack.
			assert( g_Cpu.StackLevel );
			pc = g_Cpu.Stack[ --g_Cpu.StackLevel ];

			break;

		default:
			switch ( instruction & 0xf000 )
			{
			case 0x0000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Calls RCA 1802 program at address 0x%x", addr );
					assert( 0 );
				}
				break;
			case 0x1000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Jump to 0x%x", addr );

					// Set PC, we do -2 as PC is incremented by two after each instruction.
					pc = addr - 2;
				}
				break;
			case 0x2000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Call sub routine to 0x%x", addr );

					// Store current PC on stack.
					g_Cpu.Stack[ g_Cpu.StackLevel++ ] = pc;

					// Set PC, we do -2 as PC is incremented by two after each instruction.
					pc = addr - 2;
				}
				break;
			case 0x3000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Skips next instruction if v%02d equals %d", reg, val );

					if ( g_Cpu.Regs.v[ reg ] == val )
					{
						pc += 2;
					}
				}
				break;
			case 0x4000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Skips next instruction if v%02d does not equals %d", reg, val );

					if ( g_Cpu.Regs.v[ reg ] != val )
					{
						pc += 2;
					}
				}
				break;
			case 0x5000:
				{
					int reg1 = ( instruction & 0x0f00 ) >> 8;
					int reg2 = ( instruction & 0x00f0 ) >> 4;
					DumpInstruction( "Skips next instruction if v%02d equals v%02d", reg1, reg2 );

					if ( g_Cpu.Regs.v[ reg1 ] == g_Cpu.Regs.v[ reg2 ] )
					{
						pc += 2;
					}
				}
				break;
			case 0x6000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Set v%02d to %d", reg, val );

					g_Cpu.Regs.v[ reg ] = val;
				}
				break;
			case 0x7000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Add %d to v%02d", val, reg );

					g_Cpu.Regs.v[ reg ] += val;
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
						g_Cpu.Regs.v[ reg1 ] = g_Cpu.Regs.v[ reg2 ];
						break;
					case 1:
						DumpInstruction( "Sets v%02d to v%02d OR v%02d", reg1, reg1, reg2 );
						g_Cpu.Regs.v[ reg1 ] = g_Cpu.Regs.v[ reg1 ] | g_Cpu.Regs.v[ reg2 ];
						break;
					case 2:
						DumpInstruction( "Sets v%02d to v%02d AND v%02d", reg1, reg1, reg2 );
						g_Cpu.Regs.v[ reg1 ] = g_Cpu.Regs.v[ reg1 ] & g_Cpu.Regs.v[ reg2 ];
						break;
					case 3:
						DumpInstruction( "Sets v%02d to v%02d XOR v%02d", reg1, reg1, reg2 );
						g_Cpu.Regs.v[ reg1 ] = g_Cpu.Regs.v[ reg1 ] ^ g_Cpu.Regs.v[ reg2 ];
						break;
					case 4:
						{
							DumpInstruction( "Sets v%02d to v%02d + v%02d [VF = 1 when there is a carry, 0 when not]", reg1, reg1, reg2 );
							unsigned __int16 result = g_Cpu.Regs.v[ reg1 ] + g_Cpu.Regs.v[ reg2 ];
							g_Cpu.Regs.v[ reg1 ] = result & 0xff;
							g_Cpu.Regs.v[ 0xf ] = ( result > 0xff ) ? 1 : 0;
						}
						break;
					case 5:
						DumpInstruction( "Sets v%02d to v%02d - v%02d [VF = 0 when there is a borrow, 1 when not]", reg1, reg1, reg2 );
						g_Cpu.Regs.v[ 0xf ] = ( g_Cpu.Regs.v[ reg1 ] >= g_Cpu.Regs.v[ reg2 ] ) ? 1 : 0;
						g_Cpu.Regs.v[ reg1 ] = g_Cpu.Regs.v[ reg1 ] - g_Cpu.Regs.v[ reg2 ];
						break;
					case 6:
						DumpInstruction( "Shift v%02d right by one [VF is set to the least significant bit of v%02d before the shift]", reg1, reg1 );
						g_Cpu.Regs.v[ 0xf ] = g_Cpu.Regs.v[ reg1 ] & 0x1;
						g_Cpu.Regs.v[ reg1 ] >>= 1;
						break;
					case 7:
						DumpInstruction( "Sets v%02d to v%02d - v%02d [VF = 0 when there is a borrow, 1 when not]", reg1, reg2, reg1 );
						g_Cpu.Regs.v[ 0xf ] = ( g_Cpu.Regs.v[ reg2 ] >= g_Cpu.Regs.v[ reg1 ] ) ? 1 : 0;
						g_Cpu.Regs.v[ reg1 ] = g_Cpu.Regs.v[ reg2 ] - g_Cpu.Regs.v[ reg1 ];
						break;
					case 0xe:
						DumpInstruction( "Shift v%02d left by one [VF is set to the most significant bit of v%02d before the shift]", reg1, reg1 );
						g_Cpu.Regs.v[ 0xf ] = ( g_Cpu.Regs.v[ reg1 ] & 0x80 ) ? 1 : 0;
						g_Cpu.Regs.v[ reg1 ] <<= 1;
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

					if ( g_Cpu.Regs.v[ reg1 ] != g_Cpu.Regs.v[ reg2 ] )
					{
						pc += 2;
					}
				}
				break;
			case 0xa000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Set i to 0x%x", addr );

					g_Cpu.Regs.i = addr;
				}
				break;
			case 0xb000:
				{
					address addr = instruction & 0x0fff;
					DumpInstruction( "Jump to address 0x%x plus v0", addr );

					// Set PC, we do -2 as PC is incremented by two after each instruction.
					pc = g_Cpu.Regs.v[ 0 ] + addr - 2;
				}
				break;
			case 0xc000:
				{
					int reg = ( instruction & 0x0f00 ) >> 8;
					int val = instruction & 0x00ff;
					DumpInstruction( "Set v%02d to random number and %d", reg, val );

					g_Cpu.Regs.v[ reg ] = ( rand( ) % 255 ) & val;
				}
				break;
			case 0xd000:
				{
					int reg1 = ( instruction & 0x0f00 ) >> 8;
					int reg2 = ( instruction & 0x00f0 ) >> 4;
					int val = instruction & 0x000f;
					DumpInstruction( "Draw sprite at v%02d,v%02d width 8, height %d", reg1, reg2, val );

					if ( g_Api.DrawAt( g_Cpu.Regs.v[ reg1 ], g_Cpu.Regs.v[ reg2 ], val, &g_Memory[ g_Cpu.Regs.i ] ) )
					{
						g_Cpu.Regs.v[ 0xf ] = 1;
					}
					else
					{
						g_Cpu.Regs.v[ 0xf ] = 0;
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

						if ( g_Keys[ g_Cpu.Regs.v[ reg ] ] )
						{
							pc += 2;
						}

						break;
					case 0xa1:
						DumpInstruction( "Skip next instruction if key stored in v%02d is not pressed", reg );

						if ( ! g_Keys[ g_Cpu.Regs.v[ reg ] ] )
						{
							pc += 2;
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
						g_Cpu.Regs.v[ reg ] = g_Cpu.Regs.delay;
						break;
					case 0xa:
						DumpInstruction( "Key press awaited and then stored in v%02d", reg );
						assert( 0 );
						break;
					case 0x15:
						DumpInstruction( "Set delay timer to v%02d", reg );
						g_Cpu.Regs.delay = g_Cpu.Regs.v[ reg ];
						break;
					case 0x18:
						DumpInstruction( "Set sound timer to v%02d", reg );
						g_Cpu.Regs.sound = g_Cpu.Regs.v[ reg ];
						break;
					case 0x1e:
						DumpInstruction( "Sets i to i + v%02d", reg );
						g_Cpu.Regs.i += g_Cpu.Regs.v[ reg ];
						break;
					case 0x29:
						// Characters 0-F (in hexadecimal) are represented by a 4x5 font.
						DumpInstruction( "Sets i to the location of the sprite for the character in v%02d", reg );
						g_Cpu.Regs.i = g_Cpu.Regs.v[ reg ] * 5;
						break;
					case 0x33:
						{
							// With the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2.
							// In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
							DumpInstruction( "Stores Binary-coded decimal representation of v%02d in i", reg );
							int hundreds = g_Cpu.Regs.v[ reg ] / 100;
							int tens = ( g_Cpu.Regs.v[ reg ] / 10 ) % 10;
							int units = g_Cpu.Regs.v[ reg ] % 10;

							g_Memory[ g_Cpu.Regs.i + 0 ] = hundreds;
							g_Memory[ g_Cpu.Regs.i + 1 ] = tens;
							g_Memory[ g_Cpu.Regs.i + 2 ] = units;
						}
						break;
					case 0x55:
						DumpInstruction( "Stores v0 to v%02d in memory starting at i", reg );
						for ( int ix = 0; ix < reg + 1; ++ix )
						{
							g_Memory[ g_Cpu.Regs.i + ix ] = g_Cpu.Regs.v[ ix ];
						}
						//g_Cpu.Regs.i += reg + 1;
						break;
					case 0x65:
						DumpInstruction( "Fills v0 to v%02d with memory starting at i", reg );
						for ( int ix = 0; ix < reg + 1; ++ix )
						{
							g_Cpu.Regs.v[ ix ] = g_Memory[ g_Cpu.Regs.i + ix ];
						}
						//g_Cpu.Regs.i += reg + 1;
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

		previousInstruction = instruction;
		pc += 2;
	}

	g_Api.Destroy( );

	return 0;
}
