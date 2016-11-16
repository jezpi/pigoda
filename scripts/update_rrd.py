#!/usr/bin/env /usr/bin/python2

import rrdtool, time,os,sys,datetime,sqlite3;

def update_db(sqlitedb, table):
        rrdfile=rrddbpath+"/"+table+".rrd";

	st = os.stat(rrdfile);
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
        try:
	    cur.execute("SELECT * from "+table+" WHERE timestamp > ? ORDER BY timestamp", [str(st.st_mtime+5)]);
        except:
            print "Problem with sql query"

	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		try:
			rrdtool.update(rrdfile, sup);
		except Exception as e:
			print "Failed to update "+str(row[0])+" Exception code:"+str(e);

	conn.close()

#######
# main
#

dbfile="/var/db/pigoda/sensorsv2.db";
rrddbpath="/var/db/pigoda/rrd/";

if len(sys.argv) < 2:
	print "usage: update_rrd [arg]"
	sys.exit(0);
	
cmd=sys.argv[1]

update_db(dbfile, cmd);

