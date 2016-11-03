#include <wiringPi.h>
#include <softPwm.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <mosquitto.h>
#include <string.h>

static int  MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...);
int
main(int argc, char **argv)
{
	int pinnum = 6;
	int val;
	struct mosquitto *m;

	printf("\tRev. %d\n", piBoardRev());

	if (argc < 2 ) {
		printf("usage: [pwm]\n");
		printf("PIR pin defaults to %d\n", pinnum);
	} else {
		pinnum = atoi(argv[1]);
	}
	mosquitto_lib_init();
	m= mosquitto_new("pir_sensors", true, NULL);
	mosquitto_connect(m, "172.17.17.1", 1883, 300);
         	
	wiringPiSetup();

	pinMode(pinnum, INPUT);
	pinMode(0, OUTPUT);
	digitalWrite(pinnum, HIGH);
	int tick;
	int positive;
	bool actled = false;
	for (; ; ) {
		for (tick = 0, positive=0; tick < 60; tick++) {
			if ((val = digitalRead(pinnum)) == HIGH) {
				positive++;
				digitalWrite(0, HIGH);
				actled = true;
				fprintf(stderr, ".");
				fflush(stderr);
			} else if (actled == true) {
				digitalWrite(0, LOW);
			}
			usleep(1000000);
			if (actled == true) {
				digitalWrite(0, LOW);
				actled = false;
			}
		}
		if (positive > 0) printf("\n");
		else
			printf("|\n");
		MQTT_pub(m, "/guernika/environment/pir", false, "%f", (float)positive/60);
		/*printf("rate %f\n", (float)positive/60);*/

	}
	mosquitto_disconnect(m);
	return(0);
}
static int  
MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...)
{
	size_t msglen;
	va_list lst;
	char	msgbuf[BUFSIZ];
	int	mid = 0;
	int	ret;

	va_start(lst, fmt);
	vsnprintf(msgbuf, sizeof msgbuf, fmt, lst);
	va_end(lst);
	msglen = strlen(msgbuf);
	/*mosquitto_publish(mosq, NULL, "/guernika/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	if (mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm) == MOSQ_ERR_SUCCESS) {
		ret = mid;
	} else
		ret = -1;
	return (ret);
}
