#ifndef _MQTT_CMD_H_
#define _MQTT_CMD_H_


typedef enum {CMD_NIL, CMD_ERR, CMD_STATS, CMD_EXEC, CMD_QUIT, CMD_LED, CMD_FAN, CMD_RELAY, CMD_HALT} mqtt_cmd_t;
const char *cmd_str[] = {"CMD_NIL", "CMD_ERR", "CMD_STATS", "CMD_EXEC", "CMD_QUIT", "CMD_LED", "CMD_FAN", "CMD_RELAY", "CMD_HALT", NULL};

#endif /* ! _MQTT_CMD_H_ */
