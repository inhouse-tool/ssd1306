////////////////////////////////////////////////////////////////////////
//
//	Status Viewer
//

#include <unistd.h>
#include <stdio.h>
#include <time.h>

// SSD1306

#include "ssd1306.h"
#define	I2C_SSD1306	0x3c		// SSD1306's I2C slave address

	static	int	fdSSD;		// SSD1306's file descriptor

	static	void	AcceptSignal( void );
	static	void	SetTimer( void );

int
main( void )
{
	fdSSD = ssdSetup( I2C_SSD1306, 128, 64 );

	AcceptSignal();

	if	( fdSSD > 0 ){
		ssdFlip( fdSSD, FALSE );

		SetTimer();

		for	( ;; )
			pause();
	}

	return 0;
}


#include <sys/time.h>
#include <signal.h>

	static	void	OnSignal( int signo );

static	void
AcceptSignal( void )
{
	signal( SIGUSR1, OnSignal );
	signal( SIGUSR2, OnSignal );
}

static	void
OnSignal( int signum )
{
	if	( signum == SIGUSR1 )
		ssdSetDim( fdSSD, 127 );
	else if	( signum == SIGUSR2 )
		ssdSetDim( fdSSD,   0 );
}

	static	void	OnTimer( int signo );

static	void
SetTimer( void )
{
	struct	itimerval	val;
	struct	sigaction	act;

	sigemptyset( &act.sa_mask );
	act.sa_flags   = 0;
	act.sa_handler = OnTimer;
	sigaction( SIGALRM, &act, NULL );

	struct	timespec	ts;
	clock_gettime( CLOCK_REALTIME, &ts );
	int	us = ts.tv_nsec;
	us /= 1000;
	us = 1000000 - us;
	usleep( us );

	val.it_interval.tv_sec  = 1;
	val.it_interval.tv_usec = 0;
	val.it_value.   tv_sec  = 1;
	val.it_value.   tv_usec = 0;

	setitimer( ITIMER_REAL, &val, NULL );
}

	static	void	Update( void );

static	void
OnTimer( int signo )
{
	Update();
}

#include <stdlib.h>
#include <string.h>
#include "types.h"

	static	LLONG	userLast;
	static	LLONG	niceLast;
	static	LLONG	systLast;
	static	LLONG	idleLast;
	static	LLONG	 iowLast;
	static	LLONG	 irqLast;
	static	LLONG	sirqLast;

/*
	2x16

	12:34:56   100'C
	600MHz   100.00%

	12:34:56   100'C
	230920kB 100.00%
*/

static	void
Update( void )
{
	static	int	secLast;
	char	buf[256];

	struct	timespec	ts;
	clock_gettime( CLOCK_REALTIME, &ts );
	struct	tm	lt = *localtime( &ts.tv_sec );
	if	( lt.tm_sec != ( secLast+1 ) % 60 )
		SetTimer();

	secLast = lt.tm_sec;
	sprintf( buf, "%02d:%02d:%02d",
			lt.tm_hour, lt.tm_min, lt.tm_sec );
	ssdPuts( fdSSD, 0, 0, buf );

	if	( lt.tm_sec % 10 == 0 ){

		FILE	*fp;

		fp  = fopen( "/sys/class/thermal/thermal_zone0/temp", "r" );
		if	( fp ){
			int	temp = 0;
			fgets( buf, sizeof( buf ), fp );
			fclose( fp );

			temp = atoi( buf );
			temp = (temp+500)/1000;
			sprintf( buf, "%3d\x80", temp );
			{
				int	n = strlen( buf );
				ssdPuts( fdSSD, 16-n, 0, buf );
			}
		}

		fp  = fopen( "/proc/stat", "r" );
		if	( fp ){
			char	*p;
			fgets( buf, sizeof( buf ), fp );
			fclose( fp );

			p = buf+5;
			LLONG	userNew = strtoll( p, &p, 10 );
			LLONG	niceNew = strtoll( p, &p, 10 );
			LLONG	systNew = strtoll( p, &p, 10 );
			LLONG	idleNew = strtoll( p, &p, 10 );
			LLONG	iowNew  = strtoll( p, &p, 10 );
			LLONG	irqNew  = strtoll( p, &p, 10 );
			LLONG	sirqNew = strtoll( p, &p, 10 );
			LLONG	user = userNew - userLast;
			LLONG	nice = niceNew - niceLast;
			LLONG	syst = systNew - systLast;
			LLONG	idle = idleNew - idleLast;
			LLONG	iow  =  iowNew -  iowLast;
			LLONG	irq  =  irqNew -  irqLast;
			LLONG	sirq = sirqNew - sirqLast;
			userLast = userNew;
			niceLast = niceNew;
			systLast = systNew;
			idleLast = idleNew;
			 iowLast =  iowNew;
			 irqLast =  irqNew;
			sirqLast = sirqNew;

			LLONG	busy = user + nice + syst + iow + irq + sirq;
			LLONG	all = busy + idle;
			LLONG	rate = ( busy * 100000 ) / all;
			rate += 5;
			rate /= 10;
			sprintf( buf, "%3lld.%02lld%%", rate/100, rate%100 );
			{
				int	n = strlen( buf );
				ssdPuts( fdSSD, 16-n, 1, buf );
			}
		}

#ifdef	CPU_FREQ
		fp  = fopen( "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r" );
		if	( fp ){
			fgets( buf, sizeof( buf ), fp );
			fclose( fp );

			int	freq = atoi( buf );
			freq /= 1000;
			if	( freq < 1000 )
				sprintf( buf, "%dMHz", freq );
			else
				sprintf( buf, "%d.%dGHz", freq/1000, freq%1000 );
			ssdPuts( fdSSD, 0, 1, buf );
		}
#else
		fp  = fopen( "/proc/meminfo", "r" );
		if	( fp ){
			char	*p;

			fgets( buf, sizeof( buf ), fp );
			p = strchr( buf, ':' );
			int	total = atoi( ++p );

			fgets( buf, sizeof( buf ), fp );
			p = strchr( buf, ':' );
		//	int	free = atoi( ++p );

			fgets( buf, sizeof( buf ), fp );
			p = strchr( buf, ':' );
			int	avail = atoi( ++p );

			fclose( fp );

			sprintf( buf, "%dkB ", total-avail );
			ssdPuts( fdSSD, 0, 1, buf );
		}
#endif
	}
}
