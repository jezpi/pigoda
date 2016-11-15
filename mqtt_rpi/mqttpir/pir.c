/*
 *
 */
#include <wiringPi.h>
#include <softPwm.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <mosquitto.h>
#include <string.h>

static char *mqtt_host, *mqtt_user, *mqtt_password, *mqtt_topic;
static int mqtt_port = 1883;
static unsigned short debug_mode;
#define DEFAULT_MQTT_PIN 6

static int  MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...);
int
main(int argc, char **argv)
{
	int pinnum = DEFAULT_MQTT_PIN;
	int val;
	int opt;
	struct mosquitto *m;

	while ((opt = getopt(argc, argv, "h:u:p:P:vt:")) != -1) {
		switch(opt) {
			case 'h': /* host */
				mqtt_host = strdup(optarg);
				break;
			case 't': /* topic */
				mqtt_topic = strdup(optarg);
				break;
			case 'p': /* port */
				mqtt_port = atoi(optarg);
				break;
			case 'P': /* password */
				mqtt_password = strdup(optarg);
				break;
			case 'u': /* user */
				mqtt_user = strdup(optarg);
				break;
			case 'v':
				debug_mode++;
				break;
			default:
				printf("usage: mqttpir [-t topic] [-u user] [-h host] [-P passowrd] [-p port]\n");
				printf("PIR pin defaults to %d\n", pinnum);
				printf("\tRev. %d\n", piBoardRev());
		}
	}
	mosquitto_lib_init();
	m= mosquitto_new("mqtt_pir", true, NULL);
	mosquitto_username_pw_set(m, mqtt_user, mqtt_password);

	mosquitto_connect(m, mqtt_user, mqtt_port, 300);
         	
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
		if (debug_mode) {
			if (positive > 0) printf("\n");
			else
				printf("|\n");
		}
		MQTT_pub(m, mqtt_topic, false, "%f", (float)positive/60);

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
	/*mosquitto_publish(mosq, NULL, "/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	if (mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm) == MOSQ_ERR_SUCCESS) {
		ret = mid;
	} else
		ret = -1;
	return (ret);
}
