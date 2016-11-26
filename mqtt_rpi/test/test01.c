#include <stdio.h>
#include <mosquitto.h>


int
main(void)
{
	printf("%s\n", mosquitto_strerror(0));
}
