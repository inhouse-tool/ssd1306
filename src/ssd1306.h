#include "types.h"

	int	ssdSetup( int fd, int w, int h );
	int	ssdClear( int fd );
	int	ssdPuts( int fd, int col, int row, char *string );
	int	ssdFlip( int fd, BOOL flip );
	int	ssdDisplayOn( int fd, BOOL on );
	int	ssdSetColRow( int fd, int col, int row );
	int	ssdSetDim( int fd, BYTE dim );
	int	ssdSendCmnd( int fd, int cb, ... );
	int	ssdSendData( int fd, int cb, BYTE *pb );
