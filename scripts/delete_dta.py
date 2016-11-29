#!/usr/bin/env /usr/bin/python2


import time,os,sys,datetime,sqlite3;

def rm_rrd_file(rrdfile):
	rrd_db_path="/var/db/pigoda/rrd/";
	rrd_path=rrd_db_path+rrdfile+".rrd";
	try:
		os.stat(rrd_path);
		print "deleting "+rrd_path;
		os.remove(rrd_path);
	except :
		print "file \""+rrd_path+"\" does not exist"


def rm_sqlite3(sqlitedb, tblname):
	conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
        try:
	    cur.execute("DROP TABLE "+tblname);
        except Exception as e:
            print "Problem with sql query: "+str(e);
	conn.close()


####
# main
##

if len(sys.argv) < 2:
	print "too few arguments"
	print "usage: "+sys.argv[0]+" [dta_name]"
	sys.exit()

dta_name=sys.argv[1];

print "=> Removing "+dta_name+" rrd file";
rm_rrd_file(dta_name);
print "=> Dropping table "+dta_name;
rm_sqlite3("/var/db/pigoda/sensorsv2.db", dta_name);

