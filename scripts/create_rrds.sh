#!/bin/bash

. ./pigoda_rrd.inc.sh


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



createdb_tempin () {
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


	sqlite3 -separator " " ${SQLITE_DB_PATH} 'select timestamp,temp from temp_in where timestamp > '$offs';'|awk '{system("rrdupdate '${RRD_DB_PATH}/'tempin.rrd "$1":"$2);printf(".");}'
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

	sqlite3 -separator " " ${SQLITE_DB_PATH} 'select timestamp,temp from temp_out where timestamp > '$offs';'|awk '{system("rrdupdate '${RRD_DB_PATH}/'tempout.rrd "$1":"$2);printf(".");}'
}

createdb_pir () {
	[ -f ${RRDFILE} ] && rm -v ${RRDFILE}
	printf "=> Creating rrd db: %s %d\n" "${RRDFILE}" "$offs"
	date --date='@'"${offs}"

	rrdtool create ${RRDFILE} \
		--step 60 \
		--start $offs  \
		DS:pir:GAUGE:120:0:24000 \
		RRA:AVERAGE:0.5:1:864000 \
		RRA:AVERAGE:0.5:60:129600 \
		RRA:AVERAGE:0.5:3600:13392 


	sqlite3 -separator " " ${SQLITE_DB_PATH} 'select timestamp,pir from pir where timestamp > '$offs';'|awk '{system("rrdupdate '${RRD_DB_PATH}/'pir.rrd "$1":"$2);printf(".");}'
			
}


create_pressure () {
        [ -f ${RRDFILE} ] && rm -v ${RRDFILE}
        printf "=> Creating rrd db: %s\n" "${RRDFILE}"
        date --date='@'"${offs}"
        rrdtool create ${RRDFILE} \
                --step 60 \
                --start $offs  \
                DS:pressure:GAUGE:120:0:24000 \
                RRA:AVERAGE:0.5:1:864000 \
                RRA:AVERAGE:0.5:60:129600 \
                RRA:AVERAGE:0.5:3600:13392 

	sqlite3 -separator " " ${SQLITE_DB_PATH} 'select timestamp,pressure from pressure where timestamp > '$offs';'|awk '{system("rrdupdate '${RRD_DB_PATH}/'pressure.rrd "$1":"$2);printf(".");}'
	

}


createdb_light () {
        rm -v ${RRDFILE}
        echo "=> Creating rrd db"
        rrdtool create ${RRDFILE} \
                --step 60 \
                --start $offs  \
                DS:light:GAUGE:120:0:24000 \
                RRA:AVERAGE:0.5:1:864000 \
                RRA:AVERAGE:0.5:60:129600 \
                RRA:AVERAGE:0.5:3600:13392 

	echo "=> Filling rrd db with data"
	sqlite3 -separator " " ${SQLITE_DB_PATH} 'select times,val from light where times > '$offs';'|awk '{system("rrdupdate '${RRD_DB_PATH}/'light.rrd "$1":"$2);printf(".");}'
}



#############
# main
###

curdate=`/usr/bin/date +%s`
offs=$((curdate-43200))
#echo "$offs"
for cmd in $@
do
	case $cmd in
		tempout)
			RRDFILE="/var/db/pigoda/rrd/tempout.rrd"
			createdb_tempout;
			;;
		tempin)
			RRDFILE="/var/db/pigoda/rrd/tempin.rrd"
			createdb_tempin;
			;;
		light)
			RRDFILE="/var/db/pigoda/rrd/light.rrd"
			createdb_light;
			;;
		pressure)
			RRDFILE="/var/db/pigoda/rrd/pressure.rrd"
			create_pressure;
			;;
		pir)
			RRDFILE="/var/db/pigoda/rrd/pir.rrd"
			createdb_pir;
			;;
		*)
			echo "Invalid argument $cmd"
			echo "usage: $0 [tempin|tempout|light|pressure|pir]"
			exit 64
			;;
	esac
done
