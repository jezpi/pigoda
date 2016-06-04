#!/bin/sh 

dt=`date +%s`
offs=$((dt-10800))

echo "=> Creating rrd db"
rrdtool create light.rrd \
--step 60 \
--start  $offs  \
DS:light:GAUGE:120:0:255 \
RRA:MAX:0.5:1:1500 \

echo "=> Filling rrd db with data"
sqlite3 -separator " " /home/jez/code/MQTT/graph_channel/sensors.db 'select times,val from light where times > '$offs';'|awk '{system("rrdupdate light.rrd "$1":"$2);printf(".");}'

echo "=> Creating png graph"
rrdtool graph /var/www/pigoda/light.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start $offs --end now \
--font='DEFAULT:7:' \
--title="light" \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:temp_core=light.rrd:light:MAX \
LINE1:temp_core#0000FF:"light" \
GPRINT:temp_core:MAX:"Max\: %5.2lf" 



