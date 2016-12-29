#ifndef _MQTT_ADT_H_ 
#define  _MQTT_ADT_H_ 

typedef enum    {TP_PRINT=0x2, TP_STORE=0x4, TP_ALERT=0x8, TP_SKIP=0x10, TP_LOG=20} topic_action_t;
struct topic {
	char 	*t_name;
	topic_action_t t_action;
	struct topic *t_next;
} ;
typedef struct topic topic_t ;

typedef struct {
	topic_t 	*channel_head;
	topic_t 	*channel_tail;
} channel_set_t ;

typedef struct scr_stat {
	char 	*ss_name;
	float 	 ss_value;
	unsigned long ss_err_sqlite;
	unsigned long ss_store_sqlite;
	time_t 	ss_lastupdate;
	struct scr_stat *ss_next;
} scr_stat_t;

#include <ncurses.h>
struct mq_ch_screen {
	WINDOW *win;
	WINDOW *win_stat;
	scr_stat_t *screen_stats_head;
	scr_stat_t *screen_stats_tail;
} ;

extern scr_stat_t *scr_stat_new(struct mq_ch_screen *sd, const char *name, float value);
extern scr_stat_t *scr_stat_find(struct mq_ch_screen *sd, const char *name);
extern scr_stat_t *scr_stat_update(struct mq_ch_screen *sd, const char *name, float value);
extern topic_t *topic_add(channel_set_t *chset, char *name);
extern topic_t *topic_find(channel_set_t *chset, const char *topic);


#endif /* ! _MQTT_ADT_H_ */
