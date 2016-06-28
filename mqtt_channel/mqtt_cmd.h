#ifndef _MQTT_CMD_H_
#define _MQTT_CMD_H_

/* TODO remove static commands */
typedef enum {
  CMD_NIL,
  CMD_ERR,
  CMD_STATS,
  CMD_EXEC,
  CMD_QUIT,
  CMD_LED,
  CMD_LIGHT,
  CMD_TEMPIN,
  CMD_TEMPOUT,
  CMD_PRESSURE,
  CMD_HUMIDITY,
  CMD_PIR,
  CMD_MQ2
} mqtt_cmd_t;

const char *cmd_str[] = {
  "CMD_NIL",
  "CMD_ERR",
  "CMD_STATS",
  "CMD_EXEC",
  "CMD_QUIT",
  "CMD_LED",
  "CMD_LIGHT",
  "CMD_TEMPIN",
  "CMD_TEMPOUT",
  "CMD_PRESSURE",
  "CMD_HUMIDITY",
  "CMD_PIR",
  "MQ2",
  NULL
};

#endif /* ! _MQTT_CMD_H_ */
