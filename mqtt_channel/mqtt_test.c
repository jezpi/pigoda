
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <signal.h>
#include <unistd.h>

#include <mosquitto.h>
void my_connect_callback(struct mosquitto *, void *, int);
int
main(void)
{
	struct mosquitto *m;
	int mlret = 0;
	bool whileloop = true;

	mosquitto_lib_init();

	m = mosquitto_new("komar", false, NULL);


	mosquitto_username_pw_set(m, "jez", "bzzz.8");
	mosquitto_connect_callback_set(m, my_connect_callback);


	switch (mosquitto_connect(m, "mail.obin.org", 1883, 120)) {
		case MOSQ_ERR_SUCCESS:
			printf("connection ok\n");
			break;
		case MOSQ_ERR_INVAL:
			break;
		case MOSQ_ERR_ERRNO:
			break;
		default:
			printf("Unknown error\n");
	}	

	while(whileloop) {
		mlret = mosquitto_loop(m, 1200, 1);
		switch(mlret) {
			case MOSQ_ERR_SUCCESS:
				printf("mqloop: SUCCESS\n");
				break;
			case MOSQ_ERR_INVAL:
				printf("mqloop: INVAL\n");
				whileloop = false;
				break;
			case MOSQ_ERR_NOMEM:
				printf("mqloop: NOMEM.\n");
				whileloop = false;
				break;
			case MOSQ_ERR_NO_CONN:
				printf("mqloop: NOCONN to broker\n");
				whileloop = false;
				break;
			case MOSQ_ERR_CONN_LOST:
				printf("mqloop: conn to broker lost\n");
				whileloop = false;
				break;
			case MOSQ_ERR_PROTOCOL:
				printf("mqloop: ERR_PROTOCOL\n");
				whileloop = false;
				break;	
			case MOSQ_ERR_ERRNO:
				printf("mqloop: errno %s\n", strerror(errno));
				whileloop = false;
				break;
			case MOSQ_ERR_CONN_REFUSED:
				printf("mqloop: Connection refused\n");
				whileloop = false;
				break;
			default:
				printf("mqloop: unknown error %d\n", mlret);
				whileloop = false;
		}
	}
	mosquitto_disconnect(m);
	return(0);
}

#define CONNACK_ACCEPTED 0
#define CONNACK_REFUSED_PROTOCOL_VERSION 1
#define CONNACK_REFUSED_IDENTIFIER_REJECTED 2
#define CONNACK_REFUSED_SERVER_UNAVAILABLE 3
#define CONNACK_REFUSED_BAD_USERNAME_PASSWORD 4
#define CONNACK_REFUSED_NOT_AUTHORIZED 5

void 
my_connect_callback(struct mosquitto *m, void *obj, int rc)
{
	switch (rc) {
		case 0:
			printf("CONNACK received. Connected 100%\n");
			break;
		case 1:
			printf(" connection refused (unacceptable protocol version)\n");
			break;
		case 2:
			printf("connection refused (identifier rejected)\n");
			break;
		case 3:
			printf("connection refused (broker unavailable)\n");
			break;
		case CONNACK_REFUSED_BAD_USERNAME_PASSWORD:
			printf("connection refused. Invalid username or password\n");
			break;
		case CONNACK_REFUSED_NOT_AUTHORIZED:
			printf("connection refused. Not authorized\n");
			break;
		default:
			printf("Unknown rc=%d\n", rc);
	}
}
