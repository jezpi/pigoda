
#ifndef _MQTT_DB_H_
#define  _MQTT_DB_H_

typedef enum {T_LIGHT, T_TEMPIN, T_TEMPOUT, T_PRESSURE, T_HUMIDITY, T_PIR, T_MQ2} MQTT_data_type_t;

typedef struct {
	sqlite3 *Mdb_hnd;
} MQTT_db_t;

extern MQTT_db_t *MQTT_initdb(const char *);
extern int MQTT_detachdb(MQTT_db_t *);
extern int MQTT_store(MQTT_db_t *, MQTT_data_type_t, float);

#endif /*  ! _MQTT_DB_H_ */
