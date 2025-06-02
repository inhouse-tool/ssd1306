////////////////////////////////////////////////////////////////////////////////
// SSD1306 ( SH1106 )
//
//	128 x 64 Dot Matrix OLED/PLED Segment/Common Driver with Controller
//	( SH1106 is capable of driving 132 segments )
//
//	Common 0 +------------------------------------------------------+
//	       : |                        Page 0                        |
//	       8 +------------------------------------------------------+
//	       : |                        Page 1                        |
//	      16 +------------------------------------------------------+
//	       : |                        Page 2                        |
//	      24 +------------------------------------------------------+
//	       : |                        Page 3                        |
//	      32 +------------------------------------------------------+
//	       : |                        Page 4                        |
//	      40 +------------------------------------------------------+
//	       : |                        Page 5                        |
//	      48 +------------------------------------------------------+
//	       : |                        Page 6                        |
//	      56 +------------------------------------------------------+
//	       : |                        Page 7                        |
//	      64 +------------------------------------------------------+
//	 Segment 0 .................................................. 128 (132)

#include "ssd1306.h"
#include <errno.h>

	static	int	xPix;
	static	int	yPix;
	static	int	xOffset;

	static	int	ssdOpen( int addr );
	static	int	ssdWrite( int fd, BYTE* pb, int cb );

int
ssdSetup( int addr, int w, int h )
{
	int	cb;

	// Open the device with given address.

	int	fd = ssdOpen( addr );

	if	( fd <= 0 )
		return	-1;

	// For 32pix height dispkays: Adjust Parameters.

	if	( h == 32 ){
		// Setup page start and end address

		cb = ssdSendCmnd( fd, 3, 0x22, 0, 3 );	// Page #0 to #3
		if	( cb < 0 )
			return	-1;

		// Set Multiplex Ratio

		cb = ssdSendCmnd( fd, 2, 0xa8, 31 );	// x32
		if	( cb < 0 )
			return	-1;

		// Set COM Pins Hardware Configuration

		cb = ssdSendCmnd( fd, 2, 0xda, 0x02 );	// Disable COM L/R remap, Sequential COM pin
		if	( cb < 0 )
			return	-1;
	}

	// for 64pix height displays: Restore default.

	else if	( h == 64 ){
		// Setup page start and end address

		cb = ssdSendCmnd( fd, 3, 0x22, 0, 7 );	// Page #0 to #7
		if	( cb < 0 )
			return	-1;

		// Set Multiplex Ratio

		cb = ssdSendCmnd( fd, 2, 0xa8, 63 );	// x64
		if	( cb < 0 )
			return	-1;

		// Set COM Pins Hardware Configuration

		cb = ssdSendCmnd( fd, 2, 0xda, 0x12 );	// Disable COM L/R remap, Alternative COM pin
		if	( cb < 0 )
			return	-1;
	}	

	// for other height displays: Abandon it.

	else{
		errno = EINVAL;
		return	-1;
	}

	// for 128pix width displays: Leave default.

	if	( w == 128 ){
		xOffset = 0;
	}

	// for 128pix width displays connected to SH1106: Add horizontal offset.

	else if	( w == 132 ){
		xOffset = (132-128)/2;
	}

	// for other width displays: Abandon it.

	else{
		errno = EINVAL;
		return	-1;
	}

	xPix = w;
	yPix = h;

	do{
		// Set 'Page Start Address for Page Addressing Mode'.

		cb = ssdSendCmnd( fd, 1, 0xb0 | 0 );	// from Page 0
		if	( cb < 0 )
			break;

		// Set 'Display Start Line'.

		cb = ssdSendCmnd( fd, 1, 0x40 | 0 );	// from Line #0
		if	( cb < 0 )
			break;

		// Clear the screen.

		cb = ssdClear( fd );
		if	( cb < 0 )
			break;

		// Set 'Charge Pump Setting'.

		cb = ssdSendCmnd( fd, 2, 0x8d, 0x14 );	// Enable Charge Pump
		if	( cb < 0 )
			break;

		// Turn on the display.

		cb = ssdDisplayOn( fd, TRUE );		// Display ON
		if	( cb < 0 )
			break;
	}while	( 0 );

	if	( cb < 0 )
		return	-1;
	else
		return	fd;
}

#include <stdlib.h>

#define	PAGE_HEIGHT	8	// [pix/page]

int
ssdClear( int fd )
{
	BYTE	*p = (BYTE*)malloc( xPix );
	for	( int i = 0; i < xPix; i++ )
		p[i] = 0x00;

	int	cb;
	int	numPage = yPix / PAGE_HEIGHT;
	for	( int i = 0; i < numPage; i++ ){
		cb = ssdSetColRow( fd, 0, i );
		if	( cb < 0 )
			break;
		cb = ssdSendData( fd, xPix, p );
		if	( cb < 0 )
			break;
	}

	free( p );

	if	( cb < 0 )
		return	cb;

	ssdPuts( fd, 0, 0, NULL );

	return	xPix * numPage;
}

#include "fontab.c"	// Font bitmap table

	static	char	**charCache;

#include <string.h>

int
ssdPuts( int fd, int col, int row, char *string )
{
	int	numCol  = xPix / fontWidth;
	int	numRow  = yPix / fontHeight;

	// Allocate a character cache.

	if	( !charCache ){
		charCache = (char**)calloc( numRow, sizeof( char *) );
		for	( int i = 0; i < numRow; i++ )
			charCache[i] = (char*)calloc( numCol, sizeof( char ) );	
	}

	// Clear the character cache.

	if	( !string ){
		for	( int i = 0; i < numRow; i++ )
			memset( charCache[i], 0, numCol );
		return	0;
	}

	// Ommit the characters hit cache.

	int	xEnd = strlen( string );
	if	( col + xEnd > numCol ||
		  row        > numRow ){
		errno = EINVAL;
		return	-1;
	}
	{
		int	x;
		for	( x = 0; x < xEnd; x++ )
			if	( charCache[row][col+x] != string[x] )
				break;
		string += x;
		col    += x;
		xEnd   -= x;
		for	( x = xEnd-1; x >= 0; x-- )
			if	( charCache[row][col+x] != string[x] )
				break;
		xEnd = x+1;
		if	( xEnd == 0 )
			return	0;

		for	( x = 0; x <= xEnd; x++ )
			charCache[row][col+x] = string[x];
	}

	// Draw the text at given column and row.

	int	ny = fontHeight / PAGE_HEIGHT;
	row *= ny;
	col *= fontWidth;

	int	cb = 0;
	BYTE	*pb = (BYTE*)malloc( fontWidth * xEnd );
	for	( int y = 0; y < ny; y++ ){
		cb = ssdSetColRow( fd, col, row+y );
		if	( cb < 0 )
			break;

		int	ib = 0;
		for	( int x = 0; x < xEnd; x++ ){
			int	c = string[x];
			c *= fontHeight * (fontWidth/8);
			c += y;
			for	( int i = 0; i < fontWidth; i++ ){
				pb[ib++] = fontab[c];
				c += ny;
			}
		}
		cb = ssdSendData( fd, ib, pb );
		if	( cb < 0 ){
			xEnd = -1;
			break;
		}
	}
	free( pb );

	if	( cb < 0 )
		return	-1;

	return	xEnd;
}

int	
ssdDisplayOn( int fd, BOOL on )
{
	int	cb;

	// Select 'Set Display ON/OFF'.

	cb = ssdSendCmnd( fd, 1, on? 0xaf: 0xae );	// ON : OFF
	if	( cb < 0 )
		return	-1;

	return	cb;
}

int
ssdFlip( int fd, BOOL flip )
{
	int	cb;

	// Select 'Set Segment Re-map'.

	cb = ssdSendCmnd( fd, 1, flip? 0xa1: 0xa0 );	// Remapped : Normal
	if	( cb < 0 )
		return	-1;

	// Select 'Set COM Output Scan Direction'.

	cb = ssdSendCmnd( fd, 1, flip? 0xc8: 0xc0 );	// Remapped : Normal
	if	( cb < 0 )
		return	-1;

	return	2;
}

int
ssdSetColRow( int fd, int col, int row )
{
	int	cb;

	// Add offset for SH1106 implementations.

	col += xOffset;

	// Set 'Set Lower Column Start Address'.

	cb = ssdSendCmnd( fd, 1, 0x00 | ( col & 0x0f ) );
	if	( cb < 0 )
		return	-1;

	// Set 'Set Higher Column Start Address'.

	cb = ssdSendCmnd( fd, 1, 0x10 | ( col >> 4 ) );
	if	( cb < 0 )
		return	-1;

	// Set 'Set Page Start Address'.

	cb = ssdSendCmnd( fd, 1, 0xb0 | row );
	if	( cb < 0 )
		return	-1;

	return	3;
}

int
ssdSetDim( int fd, BYTE dim )
{
	int	cb;

	// Set 'Set Contrast Control'.

	cb = ssdSendCmnd( fd, 2, 0x81, dim );
	if	( cb < 0 )
		return	-1;

	return	cb;
}

#include <stdarg.h>
#define	_countof(array)	(sizeof(array)/sizeof(*array))

int
ssdSendCmnd( int fd, int cb, ... )
{
	BYTE	d[8];
	va_list	ap;

	va_start( ap, cb );

	if	( cb > _countof( d ) ){
		errno = EINVAL;
		return	-1;
	}

	int	i = 0;
	d[i++] = 0x00;				// Co = 0: Data follows / D/C# = 0: Command
	for	( int ib = 0; ib < cb; ib++ )
		d[i++] = (BYTE)va_arg( ap, int );
	va_end( ap );

	if	( ssdWrite( fd, d, 1+cb ) != 1+cb ){
		errno = EIO;
		return	-1;
	}

	return	cb;
}

int
ssdSendData( int fd, int cb, BYTE *pb )
{
	BYTE	*d;

	d = (BYTE*)malloc( cb+1 );
	memcpy( &d[1], pb, cb );
	d[0] = 0x40;				 // Co = 0: Data follows / D/C# = 1: Data
	int	cbDone = ssdWrite( fd, d, cb+1 );
	free( d );

	if	( cbDone != 1+cb ){
		errno = EINVAL;
		return	-1;
	}

	return	cb;
}

// Device dependent

#include "i2c.h"

static	int
ssdOpen( int addr )
{
	return	i2cOpen( addr );
}

static	int
ssdWrite( int fd, BYTE* pb, int cb )
{
	return	i2cWrite( fd, pb, cb );
}
