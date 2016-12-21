#!/usr/bin/env /usr/bin/python2

import rrdtool,time,os,sys,datetime,sqlite3;
PNG_GRAPH_PATH=os.getenv("PNG_GRAPH_PATH", "/var/www/pigoda");


def printoff(off):
	return time.strftime("%H:%M:%S - %d %b",time.gmtime(off))


def graph_unknown(ds_name, startdef='now-1d', enddef='now-60s'):
	rrdtool.graph(PNG_GRAPH_PATH+"/"+ds_name+".png", 
        	'--title', 'sensor '+ds_name+' statistics',
		'--start', startdef,
		'--end', enddef,
		'--font', 'DEFAULT:7:Ubuntu' ,
                '--font','TITLE:11:Ubuntu bold' ,
                '--font','UNIT:7:Ubuntu' ,
                '--font','WATERMARK:8:Ubuntu' ,
                '--font','LEGEND:9:Ubuntu'  ,
                '--color','BACK#FAFAFA' ,
		'--slope-mode',
		'--border', '0',
		'--grid-dash', '1:3',
		'--alt-y-grid',
		'--alt-autoscale',
		'--watermark', startdef+"\t"+enddef,
		'-w', '785',
		'-h', '160',
		'-a', 'PNG',
		'DEF:uval='+ds_name+'.rrd:'+ds_name+':AVERAGE',
		'VDEF:uvalavg=uval,AVERAGE',
		'VDEF:uvalmin=uval,MINIMUM',
		'VDEF:uvallast=uval,LAST',
		'VDEF:uvalmax=uval,MAXIMUM',
     		'AREA:uval#988FDC:values ',
                'LINE1:uvallast#A50004:Last value:dashes',
                'LINE1:uvalmin#FFD63F:Minimum value:dashes',
                'LINE1:uvalmax#12B209:Maximum value\l:dashes',
                'GPRINT:uval:AVERAGE:Avg\:  %5.2lf ',
                'GPRINT:uval:LAST:Last\: %5.2lf ',
                'GPRINT:uval:MIN:Min\:  %5.2lf ',
                'GPRINT:uval:MAX:Max\:  %5.2lf ' 

	);

def alltables(db_conn):
	cur = db_conn.cursor();
        try:
	    cur.execute("SELECT name FROM sqlite_master WHERE type='table';");
        except Exception as e:
            print "Problem with sql query: "+str(e)
	l = []
	for row in cur:
		l.append(row[0]);

	return l;



def get_sqlite_offset(conn, table, ts='timestamp'):
	#conn = sqlite3.connect(sqlitedb);
	cur  = conn.cursor();
	sqquery = "SELECT timestamp FROM "+str(table)+" ORDER BY timestamp DESC LIMIT 1;";
	cur.execute(sqquery);
	try:
		offset=cur.fetchone()[0];
	except:
		print "Failed to get offset"+sqquery;
		exit();
		offset=-1;
	return int(offset);


#######
# main
#

dbfile=os.getenv("SQLITE_DB_PATH", "/var/db/pigoda/sensorsv2.db");
rrddbpath=os.getenv("RRD_DB_PATH","/var/db/pigoda/rrd/");
tablestats = {}
rrd_stats = {}

if len(sys.argv) < 2:
	print "usage: check_updates [ds_name]"
else:
	ds_name=sys.argv[1]

conn = sqlite3.connect(dbfile);

st=os.stat(dbfile)
print "Size of Sqlite3 db file: "+str((st.st_size/1024)/1024)+"MB"
os.chdir(rrddbpath)
for rrd in os.listdir(rrddbpath):
	st=os.stat(rrd)
	rrd_ds_name =rrd.split(".")[0]
	#print "\t"+rrd+"\t"+str((st.st_size/1024))+"kB"
	rrd_stats[rrd_ds_name] = rrdtool.info(rrd);

#offset_s = printoff(lastupdate);
for tbl in alltables(conn):
	lastupdate = get_sqlite_offset(conn, tbl);
	tablestats[tbl] = int(lastupdate);

for tbl in sorted(tablestats, key=tablestats.__getitem__, reverse=True):
	try:
		if (ds_name == tbl):
			print "%20s\t%s\t|\t%s" % (tbl, printoff(tablestats[tbl]), printoff(rrd_stats[tbl]['last_update']));
			graph_unknown(ds_name);
	except NameError:
		print "%20s\t%s\t|\t%s" % (tbl, printoff(tablestats[tbl]), printoff(rrd_stats[tbl]['last_update']));
		


