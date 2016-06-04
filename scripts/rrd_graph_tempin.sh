#!/bin/sh 

dt=`date +%s`
offs=$((dt-10800))

echo "=> Creating rrd db"
rrdtool create temperature_in.rrd \
--step 60 \
--start  $offs  \
DS:pl:GAUGE:120:0:100 \
RRA:MAX:0.5:1:1500 \

echo "=> Filling rrd db with data"
sqlite3 -separator " " /home/jez/code/MQTT/graph_channel/sensors.db 'select timestamp,temp from temp_in where timestamp > '$offs';'|awk '{system("rrdupdate temperature_in.rrd "$1":"$2);printf(".");}'

echo "=> Creating png graph"
rrdtool graph /var/www/pigoda/tempin.png \
-w 785 -h 120 -a PNG \
--slope-mode \
--start $offs --end now \
--font='DEFAULT:7:' \
--title="vc_core temperature " \
--watermark="Date `date`" \
--alt-y-grid \
--rigid \
DEF:temp_core=temperature_in.rrd:pl:MAX \
LINE1:temp_core#0000FF:"temp(C)" \
GPRINT:temp_core:MAX:"Max\: %5.2lf" 



