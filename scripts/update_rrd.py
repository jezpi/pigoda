#!/usr/bin/env /usr/bin/python2

import rrdtool, time,os,sys,datetime,sqlite3;

def printoff(off):
	return time.strftime("%c",time.gmtime(off))

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
		print sup;
		try:
			rrdtool.update(rrdfile, sup);
		except Exception as e:
			print "Failed to update "+str(row[0])+" Exception code:"+str(e);

	conn.close()



def create_rrd_db(dsname, offs, step=60 ):
        rrdf=rrddbpath+"/"+dsname+".rrd"
	try:
		rrdtool.create(str(rrdf), "--step", str(step), "--start", str(offs),
		"DS:"+str(dsname)+":GAUGE:120:0:24000",
                "RRA:AVERAGE:0.5:1:864000",
                "RRA:AVERAGE:0.5:60:129600",
                "RRA:AVERAGE:0.5:3600:13392")
	except Exception as e:
        	print  "Failed to create a new RRD file: "+rrdf+" with time:"+str(offs)+":"+"     "+str(e);
		exit();
		




def fill_rrd_db(conn, query, rrdfile, offset):
	#conn = sqlite3.connect(sqlitedb);
	cur = conn.cursor();
	failures = 0;
	qcounter = 0;
	tim=0;
	
	cur.execute(query);
	for row in cur:
		qcounter=qcounter+1;
		tim=row[0];
		sup=str(row[0])+":"+str(row[1]);
		supp=printoff(tim)+" = "+str(row[1]);
		print(supp+"\t"+str(qcounter));
		try:
			rrdtool.update(str(rrdfile), sup);
		except Exception as e:
			failures=failures+1;
			print( "failure "+str(failures)+" /"+supp);
			print("ERR: "+str(e));

def get_sqlite_offset(conn, table, ts='timestamp'):
	#conn = sqlite3.connect(sqlitedb);
	cur  = conn.cursor();
	sqquery = "SELECT timestamp FROM "+str(table)+" ORDER BY timestamp ASC LIMIT 1;";
	cur.execute(sqquery);
	try:
		offset=cur.fetchone()[0];
	except:
		print "Failed to get offset"+sqquery;
		exit();
		offset=-1;
	return int(offset);

def plot_table(sqlitedb, tname):
    conn = sqlite3.connect(sqlitedb);
    rrdf=rrddbpath+"/"+tname+".rrd";
    offs=get_sqlite_offset(conn, tname);
    print "   Start offset:"+printoff(offs)+" "+tname;
    printoff(offs)
    if offs < 0:
	return 0;

    create_rrd_db(tname, offs, 60);
    sq_query = 'SELECT * from '+tname+' ORDER BY timestamp ASC';
    fill_rrd_db(conn, sq_query, rrdf, offs)



def update_create_ds(ds_name):
	cmd = ds_name
	try:
		os.stat(rrddbpath+"/"+cmd+".rrd")
		update_db(dbfile, cmd);
	except:
		# if file does not exist create it with *create_rrd_db* and fill via *fill_rrd_db*
		plot_table(dbfile, cmd)

#######
# main
#


dbfile=os.getenv("SQLITE_DB_PATH", "/var/db/pigoda/sensorsv2.db");
rrddbpath=os.getenv("RRD_DB_PATH","/var/db/pigoda/rrd/");

if len(sys.argv) < 2:
	print "usage: update_rrd [arg]"
	sys.exit(0);
	
cmd=sys.argv[1]


update_create_ds(cmd);
