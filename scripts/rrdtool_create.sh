#!/bin/sh
### change to the script directory


rrdtool create temperature.rrd \
--step 60 \
--start 1462642702 \
DS:pl:GAUGE:120:0:100 \
RRA:MAX:0.5:1:1500 \
