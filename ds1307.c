
#include "main.h"
#include "ds1307.h"
#include "i2c.h"
#include <xc.h>

/* 
 * DS1307 Slave address
 * D0  -  Write Mode
 * D1  -  Read Mode
 */

void init_ds1307(void)
{
    /* Stop the clock (set CH bit) */
    write_ds1307(SEC_ADDR, 0x80);

    /* Set time to 00:00:00 in 24-hour format */
    write_ds1307(SEC_ADDR, 0x00);   // 00 sec, CH = 0
    write_ds1307(MIN_ADDR, 0x00);   // 00 min
    write_ds1307(HOUR_ADDR, 0x00);  // 00 hr (24-hour mode)

    /* Control Register */
    write_ds1307(CNTL_ADDR, 0x00);  // Disable SQW (simple mode)
}

void write_ds1307(unsigned char address, unsigned char data)
{
	i2c_start();
	i2c_write(SLAVE_WRITE);
	i2c_write(address);
	i2c_write(data); //ASSSIGNED TO SSPBUF 
	i2c_stop();
}

unsigned char read_ds1307(unsigned char address)
{
	unsigned char data;

	i2c_start();
	i2c_write(SLAVE_WRITE); // IN WRITE MODE TO GET ADDRESS
	i2c_write(address);
	i2c_rep_start(); //CHANGE TO READ MODE
	i2c_write(SLAVE_READ);
	data = i2c_read();
	i2c_stop();

	return data;
}