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
#define DEFAULT_PIR_PIN 6
#define DEFAULT_LED_PIN 0
static void usage(void);

static int  MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...);
int
main(int argc, char **argv)
{
	int pirpin = DEFAULT_PIR_PIN;
	int ledpin = DEFAULT_LED_PIN;
	char *identity = "mqtt_pir";
	int val;
	int opt;
	struct mosquitto *m;

	if (argc < 2) {
		usage();
		exit(64);
	}

	while ((opt = getopt(argc, argv, "i:a:h:l:u:p:P:vt:")) != -1) {
		switch(opt) {
			case 'i':
				identity = strdup(optarg);
				break;
			case 'a':
				pirpin = atoi(optarg);
				break;
			case 'h': /* host */
				mqtt_host = strdup(optarg);
				break;
			case 'l':
				ledpin = atoi(optarg);
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
				usage();
				exit(64);
		}
	}
	mosquitto_lib_init();
	m= mosquitto_new(identity, true, NULL);
	mosquitto_username_pw_set(m, mqtt_user, mqtt_password);

	if (mosquitto_connect(m, mqtt_host, mqtt_port, 300) != MOSQ_ERR_SUCCESS) {
		if (debug_mode) 
			printf("connect failure %s@%s:%d\n", mqtt_user, mqtt_host, mqtt_port);
		exit(3);
	} else {
		printf("Connected to %s\n", mqtt_host);
	}
         	
	wiringPiSetup();

	pinMode(pirpin, INPUT);
	pinMode(ledpin, OUTPUT); /* Pin 0? */
	digitalWrite(pirpin, HIGH);

	int tick;
	int positive;
	bool actled = false;
	for (; ; ) {
		for (tick = 0, positive=0; tick < 60; tick++) {
			if ((val = digitalRead(pirpin)) == HIGH) {
				positive++;
				digitalWrite(ledpin, HIGH);
				actled = true;
				fprintf(stderr, ".");
				fflush(stderr);
			} else if (actled == true) {
				digitalWrite(ledpin, LOW);
			}
			usleep(1000000);
			if (actled == true) {
				digitalWrite(ledpin, LOW);
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

static void 
usage(void)
{
	printf("usage: mqttpir [-l ledpin] [-i identity] [-a pin] [-t topic] [-u user] [-h host] [-P passowrd] [-p port]\n");
	printf("\tPIR pin defaults to %d\n", DEFAULT_PIR_PIN);
	printf("\tLED pin defaults to %d\n", DEFAULT_LED_PIN);
	printf("\tDO NOT CHANGE THESE VALUES UNLESS YOU KNOW WHAT YOU'RE DOING\n");
	printf("Board Rev. %d\n", piBoardRev());
	return;
}
