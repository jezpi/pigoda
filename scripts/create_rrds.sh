#!/bin/bash


create_vc_tempb () {
	offset=$1
echo "=> Creating rrd db"
	rm -v temperature.rrd
	rrdtool create temperature.rrd \
		--step 60 \
		--start $offset  \
		DS:vctemp:GAUGE:120:0:24000 \
		RRA:AVERAGE:0.5:1:864000 \
		RRA:AVERAGE:0.5:60:129600 \
		RRA:AVERAGE:0.5:3600:13392 
}



createdb_tempIn () {
	rm -v ${RRDFILE}
	printf "=> Creating rrd db: %s %d\n" "${RRDFILE}" "$offs"
	date --date='@'"${offs}"
	rrdtool create ${RRDFILE} \
		--step 60 \
		--start $offs  \
		DS:tempin:GAUGE:120:0:24000 \
		RRA:AVERAGE:0.5:1:864000 \
		RRA:AVERAGE:0.5:60:129600 \
		RRA:AVERAGE:0.5:3600:13392 


}

createdb_tempout () {
	rm -v ${RRDFILE}
	printf "=> Creating rrd db: %s %d\n" "${RRDFILE}" "$offs"
	date --date='@'"${offs}"
	rrdtool create ${RRDFILE} \
		--step 60 \
		--start $offs  \
		DS:tempout:GAUGE:120:0:24000 \
		RRA:AVERAGE:0.5:1:864000 \
		RRA:AVERAGE:0.5:60:129600 \
		RRA:AVERAGE:0.5:3600:13392 


}

