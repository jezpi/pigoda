#include <stdio.h>
#include <unistd.h>
#include <sqlite3.h>
#include <stdlib.h>
#include "mqtt_db.h"
#include <time.h>

/* sourced from mqtt_channel.c */
extern int MQTT_printf(const char *, ...);
extern int MQTT_log(const char *, ...);

MQTT_db_t *
MQTT_initdb(const char *path) {
	MQTT_db_t *mq_db;
	sqlite3 *db;
	sqlite3_open(path, &db);
	mq_db = malloc(sizeof (MQTT_db_t));
	mq_db->Mdb_hnd = db;
	return (mq_db);
}

MQTT_detachdb(MQTT_db_t *mq_db) {
	sqlite3_close(mq_db->Mdb_hnd);
}

int
MQTT_store(MQTT_db_t *mq_db, MQTT_data_type_t type, float value) 
{
	char sql_buf[BUFSIZ];
	char errbuf;
	time_t	curtim;
	int ret = -1;
	char *sqliteErrMsg = 0;

	time(&curtim);

	switch (type) {
		case T_LIGHT:
			snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s VALUES (%d, %d);", "light", curtim, (int)value) ;
			ret = sqlite3_exec(mq_db->Mdb_hnd, sql_buf, NULL, 0, &sqliteErrMsg);
			if (ret != SQLITE_OK) {
				MQTT_log("Sqlite err while storing light: %s  ", sqliteErrMsg);
				sqlite3_free(sqliteErrMsg);
			}
		break;
		case T_TEMPIN:
			snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s VALUES (%d, %f);", "temp_in", curtim, value) ;
			ret = sqlite3_exec(mq_db->Mdb_hnd, sql_buf, NULL, 0, &sqliteErrMsg );
			if (ret != SQLITE_OK) {
				MQTT_log("Sqlite err while storing tempin: %s  ", sqliteErrMsg);
				sqlite3_free(sqliteErrMsg);
			}
		break;
		case T_TEMPOUT:
			snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s VALUES (%d, %f);", "temp_out", curtim, value) ;
			ret = sqlite3_exec(mq_db->Mdb_hnd, sql_buf, NULL, 0,&sqliteErrMsg  );
			if (ret != SQLITE_OK) {
				MQTT_log("Sqlite err while storing tempout: %s  ", sqliteErrMsg);
				sqlite3_free(sqliteErrMsg);
			}
		break;

		case T_PRESSURE:
			snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s VALUES (%d, %f);", "pressure", curtim, value) ;
			ret = sqlite3_exec(mq_db->Mdb_hnd, sql_buf, NULL, 0, &sqliteErrMsg );
			if (ret != SQLITE_OK) {
				MQTT_log("Sqlite err while storing pressure: %s  ", sqliteErrMsg);
				sqlite3_free(sqliteErrMsg);
			}
			break;
		case T_HUMIDITY:
			snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s VALUES (%d, %f);", "humidity_rf", curtim, value) ;
			ret = sqlite3_exec(mq_db->Mdb_hnd, sql_buf, NULL, 0, &sqliteErrMsg );
			if (ret != SQLITE_OK) {
				MQTT_log("Sqlite err while storing humidity: %s  ", sqliteErrMsg);
				sqlite3_free(sqliteErrMsg);
			}
		break;
		case T_PIR:
			snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s VALUES (%d, %f);", "pir", curtim, value) ;
			ret = sqlite3_exec(mq_db->Mdb_hnd, sql_buf, NULL, 0, &sqliteErrMsg );
			if (ret != SQLITE_OK) {
				MQTT_log("Sqlite err while storing pir: %s  ", sqliteErrMsg);
				sqlite3_free(sqliteErrMsg);
			}
		break;
		default:
			MQTT_log("SQLITE3: Unknown type %d\n", type);
			break;
		
	}
	if (ret == SQLITE_OK) {
		ret = 0;
	} else {
		ret = -1;
	}
	return (ret);
}
