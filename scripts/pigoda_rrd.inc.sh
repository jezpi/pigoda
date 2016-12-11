

# path to sqlite3 database
: ${SQLITE_DB_PATH:="/var/db/pigoda/sensors.db"}
: ${RRD_DB_PATH:="/var/db/pigoda/rrd"}
: ${RRD_FILE_TEMPIN:="/var/db/pigoda/rrd/tempin.rrd"}
: ${RRD_FILE_TEMPOUT:="/var/db/pigoda/rrd/tempout.rrd"}
: ${PNG_GRAPH_PATH:="/var/www/pigoda/"}
: ${WEB_GRAPH_IMG_CLASS:="img-responsive center-block"}
export PNG_GRAPH_PATH
export WEB_GRAPH_IMG_CLASS
