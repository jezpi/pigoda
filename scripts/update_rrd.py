#!/usr/bin/env /usr/bin/python2

import rrdtool, time,os,sys,datetime,sqlite3;


def update_db_vc_temp(sqlitedb):
	conn = sqlite3.connect(sqlitedb);
	st = os.stat('/var/db/rrdcache/vc_temp.rrd')
	print str(st.st_mtime);
	cur = conn.cursor();
	cur.execute("SELECT * from vc_temp_now WHERE qtime > ?", [str(st.st_mtime)]);
	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update('/var/db/rrdcache/vc_temp.rrd', sup);
	conn.close()




def update_db(sqlitedb, table):
        rrdfile=rrddbpath+"/"+table+".rrd";

	st = os.stat(rrdfile);
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
        try:
	    cur.execute("SELECT * from "+table+" WHERE timestamp > ?", [str(st.st_mtime)]);
        except:
            print "Problem with sql query"

	for row in cur:
		sup=str(row[0])+":"+str(row[1]);
		#print sup;
		rrdtool.update(rrdfile, sup);
	conn.close()

#######
# main
#

dbfile="/var/db/pigoda/sensorsv2.db";
rrddbpath="/var/db/pigoda/rrd/";
cmd=sys.argv[1]
if cmd == "vc_temp":
	print "=> Updating vc_temp";
	try:
		update_db_vc_temp('/var/db/pigoda/vc_temp.db')
	except:
		print "Failed to update vc_temp"
elif cmd == "tempin":
	print "=> Updating "+cmd;
        try:
	    update_db(dbfile, cmd)
        except:
            print "failed to update "+cmd

elif cmd == "tempout":
	print "=> Updating "+cmd;
        try:
	    update_db(dbfile, cmd)
        except:
            print "failed to update "+cmd

elif cmd == "light":
	print "=> Updating "+cmd;
        try:
	    update_db(dbfile, cmd)
        except:
            print "failed to update "+cmd

elif cmd == "pressure":
	print "=> Updating "+cmd;
        try:
	    update_db(dbfile, cmd)
        except:
            print "failed to update "+cmd

elif cmd == "pir":
	print "=> Updating "+cmd;
        try:
	    update_db(dbfile, cmd)
        except:
            print "failed to update "+cmd

elif cmd == "mic":
	print "=> Updating "+cmd;
        try:
	    update_db(dbfile, cmd)
        except:
            print "failed to update "+cmd

else:
	print "unknown arg"

