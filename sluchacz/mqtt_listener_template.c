/*
 */
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
#include <yaml.h>
#include <sqlite3.h>


#include <ncurses.h>
char *message;
#include "../mqtt_channel/mqtt.h"

#define HOST_NAME_MAX 64
typedef struct mqtt_hnd {
	struct mosquitto	*mqh_mos;
#define MAX_MSGBUF_SIZ	BUFSIZ
#define MAX_ID_SIZ	BUFSIZ
	char			 mqh_msgbuf[MAX_MSGBUF_SIZ];
	char 			 mqh_id[MAX_ID_SIZ];
	bool 			 mqh_clean_session;
	time_t			 mqh_start_time;
} mqtt_hnd_t;
mqtt_global_cfg_t	 myMQTT_conf;
static bool 		 proc_command;
static bool 		 verbose_mode;

static int MQTT_loop(void *m, int timeout);
static bool main_loop = true;
static bool 		 mqtt_conn_dead = false;
static FILE 		*logfile;
const char 		*__PROGNAME = "mqtt_sluchacz";
const char 		*__HOSTNAME;
static sig_atomic_t 	got_SIGTERM;
static sig_atomic_t 	got_SIGUSR1;
static bool mqtt_rpi_init(const char *progname, char *conf);
static int MQTT_sub(struct mosquitto *m, const char *fmt_topic, ...);
static void MQTT_finish(mqtt_hnd_t *m);
static int MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...);

int MQTT_debug(const char *fmt, ...);
static struct mosquitto * MQTT_init(mqtt_hnd_t *m, bool c_sess, const char *id);
void
siginfo(int signo, siginfo_t *info, void *context)
{
	MQTT_debug("%d %s %s\n", signo, context, strsignal(signo));
	fflush(stdout);
	main_loop = false;
	return ;
}
static void
sig_hnd(int sig)
{
	switch (sig) {
		case SIGTERM:
		case SIGINT:
		case SIGHUP:
		case SIGQUIT:
			got_SIGTERM = 1;
			break;
		case SIGUSR1:
			got_SIGUSR1 = 1;
			break;
		default:
			MQTT_debug("got unknown signal %s\n", strsignal(sig));
			fflush(stderr);
	}
	return;
}

char *select_ds;

int MQTT_printf(const char *, ...);
int MQTT_log(const char *, ...);
static void register_callbacks(struct mosquitto *mosq);

int
main(int argc, char **argv) {
	int ch;
	char *topic = NULL;
	mqtt_hnd_t mosq;
	logfile = fopen("/dev/null", "w");
	myMQTT_conf.mqtt_port = 1883;

	while ((ch = getopt(argc, argv, "d:u:P:i:t:h:v")) != -1) {
		 switch (ch) {
			 case 'd':
				 select_ds = strdup(optarg);
				 break;
			 case 'v':
				 verbose_mode = true;
				 logfile = stderr;
				 break;
			 case 'h':
				myMQTT_conf.mqtt_host = strdup(optarg);
				 break;
			 case 'u':
				 myMQTT_conf.mqtt_user = strdup(optarg);
				 break;
			case 'P':
				 myMQTT_conf.mqtt_password = strdup(optarg);
				 break;
			case 'i':
				 myMQTT_conf.identity = strdup(optarg);
				 break;
			case 't':
				 topic = strdup(optarg);
				 break;
		 }
	}
	if (myMQTT_conf.identity == NULL) {
		fprintf(stderr, "Too few arguments. Identity is missing\n");
		exit(64);
	} else if (myMQTT_conf.mqtt_user == NULL) {
		fprintf(stderr, "Too few arguments. User is missing\n");
		exit(64);

	} else if (myMQTT_conf.mqtt_password == NULL) {
		fprintf(stderr, "Too few arguments. Password is missing\n");
		exit(64);
	} else if (myMQTT_conf.mqtt_host == NULL) {
		fprintf(stderr, "Too few arguments. Host is missing\n");
		exit(64);
	} else {
		if (verbose_mode) printf("connecting %s@%s as %s\n", myMQTT_conf.mqtt_user,
				myMQTT_conf.mqtt_host,
				myMQTT_conf.identity);
	}

	mqtt_rpi_init(argv[0], NULL);
	MQTT_init(&mosq, true, ((myMQTT_conf.identity != NULL)?myMQTT_conf.identity:"sluchacz"));
	MQTT_sub(mosq.mqh_mos, ((topic != NULL?topic:"/environment/#")));
	MQTT_pub(mosq.mqh_mos, "/network/broadcast/sluchacz", true, "on");

	while (main_loop) {

		MQTT_loop(mosq.mqh_mos, 300000);
		if (got_SIGTERM) {
			main_loop = false;
		}
	}
	MQTT_pub(mosq.mqh_mos, "/network/broadcast/sluchacz", true, "off");

	MQTT_finish(&mosq);
}

/*
 * Call of mosquitto_loop used to select(2).
 * it wraps third argument of mosquitto_loop
 * the first argument void *m is casted to struct mosquitto
 * the second argument is a timeout expressed in milliseconds.
 */


static int
MQTT_loop(void *m, int timeout)
{
	struct mosquitto *mos = (struct mosquitto *) m;
	int	ret = 0;

	if (mqtt_conn_dead) {
		return (MOSQ_ERR_SUCCESS);
	}
	ret = mosquitto_loop(mos, timeout, 1);
	if (ret != MOSQ_ERR_SUCCESS) {
		MQTT_debug( "%s(): %s %d\n", __func__, strerror(errno), ret);
		fflush(logfile);
	}

		switch (ret) {
			case MOSQ_ERR_SUCCESS:
				;
				break;
			case MOSQ_ERR_INVAL:
				MQTT_debug( "Input parametrs invalid\n");
				main_loop = false;
				break;
			case MOSQ_ERR_NOMEM:
				MQTT_debug( "Memory condition\n");
				main_loop = false;
				break;
			case MOSQ_ERR_NO_CONN:
				MQTT_debug( "The client isn't connected to the broker\n");
				main_loop = false;
				break;
			case MOSQ_ERR_CONN_LOST:
				MQTT_debug( "Connection to the broker lost\n");
				mqtt_conn_dead = true;
				break;
			case MOSQ_ERR_PROTOCOL:
				MQTT_debug( "Protocol error in communication with broker\n");
				break;
			case MOSQ_ERR_ERRNO:
				MQTT_debug("System call error errno=%s\n", strerror(errno));
				main_loop = false;
				break;
			default:
				MQTT_debug( "Unknown ret code %d\n", ret);
				main_loop = false;
		}
	return (ret);
}

static void my_publish_callback(struct mosquitto *mosq, void *con, int mid)
{
	MQTT_log( "DEBUG: published mid=%d\n", mid);
	return ;
}

static void
MQTT_finish(mqtt_hnd_t *m)
{
	mosquitto_disconnect(m->mqh_mos);
	mosquitto_destroy(m->mqh_mos);
	mosquitto_lib_cleanup();
	return;
}

static void
my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;
	MQTT_log( "DEBUG: Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		MQTT_log( ", %d", granted_qos[i]);
	}
	MQTT_log( "\n");
}

char * 
mqtt_poli_proc_msg(char *topic, char *payload)
{
	char	*p;
	char	*pbuf = strdup(payload);
	char	*hnam_buf;
	char	**topics;
	int	topic_cnt, n;
	char 	*ret = NULL;
	enum {ST_BEGIN, ST_LUGAR, ST_UNKNOWN_MSG, ST_ENVIRONMENT, ST_SENSOR, ST_DATA, ST_DONE, ST_ERR} pstate;

	if ((p = strchr(pbuf, '\n')) != NULL)
		*p = '\0';

	if (mosquitto_sub_topic_tokenise(topic, &topics, &topic_cnt) != MOSQ_ERR_SUCCESS) {
		return (NULL);
	}
	pstate = ST_ENVIRONMENT;
	if ((topic_cnt - 2) < 0)
		return (NULL);
	
	for (n=1; topic_cnt >= n && pstate != ST_ERR; n++) {
		switch(n) {
			case 1:
				if (pstate == ST_ENVIRONMENT && !strcasecmp(topics[n], "environment")) {
					pstate = ST_SENSOR;
				} else
					pstate = ST_UNKNOWN_MSG;

				break;
			case 2:
				if (pstate == ST_SENSOR ) {
					ret = strdup(topics[n]);
				} else 
				break;
		}
	}
	message = strdup(topics[topic_cnt - 1]);
	mosquitto_sub_topic_tokens_free(&topics, topic_cnt);
	return (ret);
}



static void
my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	MQTT_log(" %d - %s\n", level, str);
	return;
}


static void
my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg)
{
	char *dataf;
	float val;
	time_t curtim;


	if (msg->payloadlen){
		if ((dataf = mqtt_poli_proc_msg(msg->topic, msg->payload)) != NULL) {
			if (select_ds != NULL && strcasecmp(select_ds, message)) {
				return;
			} 
			printf("%s = %s\n", message, msg->payload);
		}
	} else {
		MQTT_log( "Empty message on %s\n", msg->topic);
	}

	return;
}

static void
my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;

	if (!result){
		/* Subscribe to broker information topics on successful connect. */
		/*  /mosquitto_subscribe(mosq, NULL, "/#", 0);*/
		/*mosquitto_subscribe(mosq, NULL, "/network/stations", 0);*/
		MQTT_log("Connected sucessfully %s as %s\n", 
				myMQTT_conf.mqtt_host, 
				myMQTT_conf.mqtt_user);
		MQTT_debug("Connected sucessfully to %s as \"%s\":%s\n", 
				myMQTT_conf.mqtt_host, 
				myMQTT_conf.mqtt_user,
				myMQTT_conf.identity
				);
	} else {
		switch (result) {
			case CONNACK_REFUSED_PROTOCOL_VERSION:
				MQTT_log("CONNACK failure. Protocol version was refused.\n");
				break;
			case CONNACK_REFUSED_IDENTIFIER_REJECTED:
				MQTT_log("CONNACK failure. Idnetifier has been rejected.\n");
				break;
			case CONNACK_REFUSED_SERVER_UNAVAILABLE:
				MQTT_log("CONNACK failure. Server unavailable.\n");
				break;
			case CONNACK_REFUSED_BAD_USERNAME_PASSWORD:
				MQTT_log("CONNACK failure. username or password incorrect.\n");

				break;
			case CONNACK_REFUSED_NOT_AUTHORIZED:
				MQTT_log("CONNACK failure. Not authorized.\n");
				break;
			default:
				MQTT_log("Unknown CONNACK error!\n");
		}
	}
	return;
}

static struct mosquitto *
MQTT_init(mqtt_hnd_t *m, bool c_sess, const char *id)
{
	int lv_minor;
	int lv_major;
	int lv_rev;

	mosquitto_lib_init();
	mosquitto_lib_version(&lv_major, &lv_minor, &lv_rev);
	MQTT_debug( "%s@%s libmosquitto %d.%d rev=%d\n", id, __HOSTNAME, lv_major, lv_minor, lv_rev);

	strncpy(m->mqh_id, id, sizeof(m->mqh_id));
	bzero(m->mqh_msgbuf, sizeof(m->mqh_msgbuf));
	m->mqh_clean_session = c_sess;
	if ((m->mqh_mos = mosquitto_new(id, c_sess, NULL)) == NULL)
		return (NULL);

		/*  XXX */
	mosquitto_username_pw_set(m->mqh_mos, myMQTT_conf.mqtt_user, myMQTT_conf.mqtt_password);

	register_callbacks(m->mqh_mos);
	if (mosquitto_connect(m->mqh_mos, myMQTT_conf.mqtt_host, myMQTT_conf.mqtt_port, 60) != MOSQ_ERR_SUCCESS)
		return (NULL);
	return (m->mqh_mos);
}

static void
register_callbacks(struct mosquitto *mosq)
{
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_publish_callback_set(mosq, my_publish_callback);
        mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
        mosquitto_log_callback_set(mosq, my_log_callback);
	return;
}

static int
MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...)
{
	size_t msglen;
	va_list lst;
	char	msgbuf[BUFSIZ];
	int	mid = 0;
	int	ret;

	if (mqtt_conn_dead) return (0);/* Connection lost MOSQ_ERR_CONN|MOSQ_ERR_CONN_LOST */
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

static int
MQTT_sub(struct mosquitto *m, const char *fmt_topic, ...)
{
	int msgid = 0;
	int ret;
	va_list lst;
	char	msgbuf[BUFSIZ];

	va_start(lst, fmt_topic);
	vsnprintf(msgbuf, sizeof msgbuf, fmt_topic, lst);
	va_end(lst);

	if ((ret = mosquitto_subscribe(m, &msgid, msgbuf, 0)) == MOSQ_ERR_SUCCESS)
		ret = msgid;
	else
		ret = -1;
	return (ret);

}

static struct mosquitto *
MQTT_reconnect(struct mosquitto *m, int *ret)
{
	int	mosq_ret;

	MQTT_log("Reconnecting");
	if ((mosq_ret = mosquitto_reconnect(m)) != MOSQ_ERR_SUCCESS) {
		if (ret != NULL) *ret = mosq_ret;
		return (NULL);
	}
	return (m);
}





static bool
mqtt_rpi_init(const char *progname, char *conf)
{
	bool   ret = true;
	char  *configfile, *bufp, *p;
	struct sigaction sa;

	bufp = malloc(HOST_NAME_MAX);
	if (gethostname(bufp, HOST_NAME_MAX) != -1) {
		__HOSTNAME = bufp;
	} else
		__HOSTNAME = "nil";

	/*p = basename(progname);*/


	sa.sa_handler = sig_hnd;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART ;
	signal(SIGINT, sig_hnd);
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		ret = false;
		MQTT_debug("%s@sigaction() \"%s\"\n", __PROGNAME, configfile);
	}
	signal(SIGQUIT, sig_hnd);
	signal(SIGUSR1, sig_hnd);
	signal(SIGTERM, SIG_DFL);
	signal(SIGHUP, sig_hnd);
	/*  /sa.sa_handler = sig_hnd;*/
	sa.sa_sigaction = siginfo;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGQUIT, &sa, NULL) == -1) {
		ret = false;
		MQTT_debug("%s@sigaction() \"%s\"\n", __PROGNAME, configfile);
	}

	return (ret);
}
int
MQTT_log(const char *fmt, ...)
{
	va_list vargs;
	int	ret;
	char	*p;
	char	pbuf[BUFSIZ], timebuf[BUFSIZ];
	time_t	curtime;
	struct tm *tmp;



	time(&curtime);
	tmp = localtime(&curtime);
	strftime(timebuf, sizeof timebuf, "%H:%M:%S %d-%m-%y %z", tmp);
	va_start(vargs, fmt);
	ret = vsnprintf(pbuf, sizeof pbuf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(pbuf, '\n')) != NULL) {
		*p = '\0';
	}
	/*
	 * switch (logtype) {
	 * 	case LOG_FILE:
	 * 	...
	 */
	fprintf(logfile, "%s  %s\n", timebuf, pbuf);
	fflush(logfile);
	return (ret);
}


int
MQTT_debug(const char *fmt, ...)
{
	va_list vargs;
	int 	ret;
	size_t	fmtlen;
	char	*fmtbuf, *p;
	char	pbuf[BUFSIZ];

	fmtbuf = strdup(fmt);
	fmtlen = strlen(fmtbuf);
	/*
	*(fmtbuf+fmtlen) = '\0';
	fmtlen--;
	*(fmtbuf+fmtlen) = '\0';
	*/

	va_start(vargs, fmt);
	ret = vsnprintf(pbuf, sizeof pbuf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(pbuf, '\n')) != NULL) {
		*p = '\0';
	}
	/*printf("%s\n", pbuf);*/
	if (verbose_mode == true)  {
		fprintf(stderr, "%s\n", pbuf);
	}
#ifdef MQTTDEBUG
	fprintf(logfile, "%s\n", pbuf);
	fflush(logfile);
#endif
	return (ret);
}

int
MQTT_printf(const char *fmt, ...)
{
	va_list vargs;
	int	ret;
	size_t	fmtlen;
	char	*fmtbuf, *p;
	char	pbuf[BUFSIZ];

	fmtbuf = strdup(fmt);
	fmtlen = strlen(fmtbuf);
	/*
	*(fmtbuf+fmtlen) = '\0';
	fmtlen--;
	*(fmtbuf+fmtlen) = '\0';
	*/

	va_start(vargs, fmt);
	ret = vsnprintf(pbuf, sizeof pbuf, fmt, vargs);
	va_end(vargs);
	if ((p = strrchr(pbuf, '\n')) != NULL) {
		*p = '\0';
	}
		fprintf(stderr, "%s\n", pbuf);
#ifdef MQTTDEBUG
	fprintf(logfile, "%s\n", pbuf);
	fflush(logfile);
#endif
	return (ret);
}

