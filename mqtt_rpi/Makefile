DFLAGS=-DPARSER_DEBUG -DMQTTDEBUG
CFLAGS+=-g -W
CC=gcc
LDFLAGS=  -lmosquitto -lyaml -lwiringPi -lbsd
	

mqtt_rpi: mqtt_rpi.o mqtt_wiringpi.o mqtt_parse_config.o mqtt_sensors.o testBMP085.o smbus.o
	$(CC) -L/home/jez/code/lib/ $(CFLAGS) -o $@ $+  $(LDFLAGS)

mqtt_rpi.o: mqtt_rpi.c
	$(CC) $(DFLAGS) $(CFLAGS) -pipe -c $< 

mqtt_wiringpi.o: mqtt_wiringpi.c
	$(CC) $(DFLAGS) $(CFLAGS) -pipe -c $< 

smbus.o: bmp85/smbus.c
	$(CC) $(DFLAGS) $(CFLAGS) -pipe -c $< 

testBMP085.o: bmp85/testBMP085.c
	$(CC) $(DFLAGS) $(CFLAGS) -pipe -c $< 

mqtt_parse_config.o: mqtt_parse_config.c
	$(CC) $(DFLAGS) $(CFLAGS) $(DFLAGS) -pipe -c $<  


mqtt_sensors.o: mqtt_sensors.c
	$(CC) $(DFLAGS) $(CFLAGS) $(DFLAGS) -pipe -c $<  



mqtt_parser_test.o: mqtt_parser_test.c
	$(CC) $(CFLAGS) -pipe -c $<  

clean: 
	@rm -v *.o mqtt_rpi 

parser: mqtt_parse_config.o mqtt_parser_test.o
	@ $(CC) $(CFLAGS) $+ -o $@ -lyaml 
	@echo "Executing parser"
	@./parser


install: mqtt_rpi
	install -o root -m 755 mqtt_rpi /usr/bin/mqtt_rpi
#	install -o root -m 644 mqtt_rpi.yaml /etc/mqtt_rpi.yaml
	install -o root -m 755 pigoda_startup.sh /etc/init.d/pigoda
	update-rc.d pigoda defaults
	update-rc.d pigoda enable


test: mqttc parser
	@echo "usage: [test|mqttc|parser|clean]"
	@echo "Building: \""$+"\""
	@echo "Building: \""$?"\""
	@echo "Building: \""$*"\""

stop:
	echo "Stopping mqtt_rpi"
	@/etc/init.d/pigoda stop

run: mqtt_rpi
	./mqtt_rpi ./mqtt_rpi.yaml

gdb: mqtt_rpi
	gdb ./mqtt_rpi 
