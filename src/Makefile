CFLAGS = -Wall -g

all:	wallclock wallseconds status test

test:		test.o i2c.o ssd1306.o
	gcc -o test test.o i2c.o ssd1306.o

wallseconds:	wallseconds.o i2c.o led7x4.o
	gcc -o wallseconds wallseconds.o i2c.o led7x4.o

wallclock:	wallclock.o i2c.o led7x4.o
	gcc -o wallclock wallclock.o i2c.o led7x4.o

status:	status.o i2c.o ssd1306.o
	gcc -o status status.o i2c.o ssd1306.o

bcp: bcp.c
	gcc -o bcp bcp.c

amb:	amb.o gpio.o i2c.o lcd1602.o led7x4.o bme280.o
	gcc -o amb amb.o gpio.o i2c.o lcd1602.o led7x4.o bme280.o

bb:	bb.o gpio.o i2c.o lcd1602.o led7x4.o bme280.o bmp180.o
	gcc -o bb bb.o gpio.o i2c.o lcd1602.o led7x4.o bme280.o bmp180.o

led:	led.c
	gcc -g -o led -Wall led.c

copy:
	scp -p *.c *.h Makefile 192.168.0.52:src/bb
