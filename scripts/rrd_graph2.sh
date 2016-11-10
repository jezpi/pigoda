#!/bin/sh 
. ./pigoda_rrd.inc.sh

dt=`date +%s`
offs=$((dt-10800))

echo "=> Creating rrd db"
rrdtool create ${RRD_DB_PATH}/light.rrd \
--step 60 \
--start  $offs  \
DS:light:GAUGE:120:0:255 \
RRA:MAX:0.5:1:1500 

echo "=> Filling rrd db with data"
sqlite3 -separator " " ${SQLITE_DB_PATH} 'select times,val from light where times > '$offs';'|awk '{system("rrdupdate '${RRD_DB_PATH}/'light.rrd "$1":"$2);printf(".");}'

if [ ! -f "${RRD_DB_PATH}/light.rrd" ]; then
	echo "${RRD_DB_PATH}/light.rrd does not exist"
	exit 3;
fi
echo "=> Creating png graph"
rrdtool graph ${RRD_GRAPH_PATH}/light.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start $offs --end now \
--font='DEFAULT:7:' \
--title="light" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:temp_core=${RRD_DB_PATH:?err}/light.rrd:light:MAX \
LINE1:temp_core#0000FF:"light" \
GPRINT:temp_core:MAX:"Max\: %5.2lf" 



