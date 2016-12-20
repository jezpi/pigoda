

# path to sqlite3 database
: ${SQLITE_DB_PATH:="/var/db/pigoda/sensorsv2.db"}
: ${RRD_DB_PATH:="/var/db/pigoda/rrd"}
: ${RRD_FILE_TEMPIN:="/var/db/pigoda/rrd/tempin.rrd"}
: ${RRD_FILE_TEMPOUT:="/var/db/pigoda/rrd/tempout.rrd"}
: ${PNG_GRAPH_PATH:="/var/www/pigoda/"}
: ${WEB_GRAPH_IMG_CLASS:="img-responsive center-block"}
export PNG_GRAPH_PATH WEB_GRAPH_IMG_CLASS SQLITE_DB_PATH RRD_DB_PATH RRD_DB_PATH

