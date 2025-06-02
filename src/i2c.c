////////////////////////////////////////////////////////////////////////////////
// I2C
//	I2C I/F driver

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2c.h"

int
i2cOpen( int addr )
{
	int	fd = open( "/dev/i2c-1", O_RDWR );
	if	( fd < 0 ){
		errno = EIO;
		return	-1;
	}

	if	( ioctl( fd, I2C_SLAVE, addr ) < 0 ){
		errno = EIO;
		return	-1;
	}

	return	fd;
}

int
i2cRead( int fd, void *p, int size )
{
	return	read( fd, p, size );
}

int
i2cWrite( int fd, void *p, int size )
{
	return	write( fd, p, size );
}

BYTE
i2cRegReadByte( int fd, BYTE reg )
{
	union	i2c_smbus_data	data;

	struct	i2c_smbus_ioctl_data	args;

	args.read_write = I2C_SMBUS_READ;
	args.command = reg;
	args.size = I2C_SMBUS_BYTE_DATA;
	args.data = &data;

	if	( ioctl( fd, I2C_SMBUS, &args ) )
		return	-1;
	else
		return	data.byte & 0xff;
}

void
i2cRegWriteByte( int fd, BYTE reg, BYTE val )
{
	union	i2c_smbus_data	data;
	data.byte = val;

	struct	i2c_smbus_ioctl_data	args;

	args.read_write = I2C_SMBUS_WRITE;
	args.command = reg;
	args.size = I2C_SMBUS_BYTE_DATA;
	args.data = &data;

	ioctl( fd, I2C_SMBUS, &args );
}

WORD
i2cRegReadWordBE( int fd, BYTE reg )
{
	BYTE	h = i2cRegReadByte( fd, reg+0 );
	BYTE	l = i2cRegReadByte( fd, reg+1 );
	WORD	w = h;
	w <<= 8;
	w |= l;

	return	w;
}

WORD
i2cRegReadWordLE( int fd, BYTE reg )
{
	BYTE	l = i2cRegReadByte( fd, reg+0 );
	BYTE	h = i2cRegReadByte( fd, reg+1 );
	WORD	w = h;
	w <<= 8;
	w |= l;

	return	w;
}
