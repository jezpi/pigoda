#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <mosquitto.h>

#include "mqtt_ADT.h"

scr_stat_t *
scr_stat_new(struct mq_ch_screen *sd, const char *name, float value)
{
	scr_stat_t 	*sp;

	sp = calloc(1, sizeof(*sp)); /* TODO check if NULL */
	sp->ss_name = strdup(name);
	sp->ss_value = value;	
	time(&sp->ss_lastupdate);

	if (sd->screen_stats_head == NULL) {
		sd->screen_stats_head = sd->screen_stats_tail = sp;
	} else {
		sd->screen_stats_tail->ss_next = sp;
		sd->screen_stats_tail = sp;
	}
	return (sp);
}


scr_stat_t *
scr_stat_find(struct mq_ch_screen *sd, const char *name)
{
	scr_stat_t *sp; 
	scr_stat_t *ret = sp = NULL;

	if ((sp = sd->screen_stats_head) == NULL) {
		return (NULL);
	}
	do {
		if (!strcasecmp(name, sp->ss_name)) {
			ret = sp;
			break;
		}
		sp = sp->ss_next;
	} while (sp != sd->screen_stats_head && sp != NULL);
	return (ret);

}

scr_stat_t *
scr_stat_update(struct mq_ch_screen *sd, const char *name, float value) 
{
	scr_stat_t *sp = NULL;

	if ((sp = sd->screen_stats_head) == NULL) {
		return (NULL);
	}
	do {
		if (!strcasecmp(name, sp->ss_name)) {
			sp->ss_value = value;
			time(&sp->ss_lastupdate);
			return (sp);
		}
		sp = sp->ss_next;
	} while (sp != sd->screen_stats_head && sp != NULL);
	return (sp);
}

topic_t *
topic_add(channel_set_t *chset, char *name)
{
	topic_t *tp;

	tp = calloc(1, sizeof(*tp));
	tp->t_name = strdup(name);

	if (chset->channel_head == NULL) {
		chset->channel_head = chset->channel_tail = tp;
	} else {
		chset->channel_tail->t_next = tp;
		chset->channel_tail = tp;
	}
	return (tp);
}

topic_t *
topic_find(channel_set_t *chset, const char *topic)
{
	topic_t *tp = NULL;
	bool match = false;

	if ((tp = chset->channel_head) == NULL)
		return (NULL);
	do {
		if (mosquitto_topic_matches_sub(tp->t_name, topic, &match) != MOSQ_ERR_SUCCESS)
			return (NULL);
		if (match == true) {
			return (tp);
		}
		tp = tp->t_next;
	} while (tp != NULL && tp != chset->channel_head);
	return (tp);
}



