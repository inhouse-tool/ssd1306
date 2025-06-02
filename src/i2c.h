#include "types.h"

		int	i2cOpen( int addr );
		int	i2cRead( int fd, void* p, int size );
		int	i2cWrite( int fd, void* p, int size );
		BYTE	i2cRegReadByte( int fd, BYTE reg );
		void	i2cRegWriteByte( int fd, BYTE reg, BYTE data );
		WORD	i2cRegReadWordBE( int fd, BYTE reg );
		WORD	i2cRegReadWordLE( int fd, BYTE reg );
