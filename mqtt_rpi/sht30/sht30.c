/* 
 * Distributed with a free-will license.
 *  Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
 * SHT30
 * This code is designed to work with the SHT30_I2CS I2C Mini Module available from ControlEverything.com.
 * https://www.controleverything.com/content/Humidity?sku=SHT30_I2CS#tabs-0-product_tabset-2
 */
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "sht30.h"

void sht30_init(void) {
	/* empty function */
	return;
}

struct sht30 *sht30_getdata(void) 
{
	// Create I2C bus
	char         *bus = "/dev/i2c-1";
	struct sht30 *dta;
	int           busfd;


	if ((busfd = open(bus, O_RDWR)) < 0) 
	{
		return (NULL);
	}
	// Get I2C device, SHT30 I2C address is 0x44(68)
	ioctl(busfd, I2C_SLAVE, 0x44);

	// Send measurement command(0x2C)
	// High repeatability measurement(0x06)
	char config[2] = {0};
	config[0] = 0x2C;
	config[1] = 0x06;
	write(busfd, config, 2);
	sleep(1);

	// Read 6 bytes of data
	// Temp msb, Temp lsb, Temp CRC, Humididty msb, Humidity lsb, Humidity CRC
	char data[6] = {0};

	if(read(busfd, data, 6) != 6) {
		printf("Erorr : Input/output Erorr \n");
	} else {
		// Convert the data
		int temp = (data[0] * 256 + data[1]);
		float cTemp = -45 + (175 * temp / 65535.0);
		float fTemp = -49 + (315 * temp / 65535.0);
		float humidity = 100 * (data[3] * 256 + data[4]) / 65535.0;

		dta = malloc(sizeof(struct sht30));
		dta->temperature = cTemp;
		dta->humidity = humidity;
	}
	close(busfd);
	return (dta);
}
