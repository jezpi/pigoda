DFLAGS=-DPARSER_DEBUG -DMQTTDEBUG
CFLAGS+=-g -W
CC=gcc
LDFLAGS=  -lmosquitto -lyaml -lsqlite3 -lncurses
	


mqtt_channel: mqtt_channel.o mqtt_parse_config.o mqttc_sqlite3.o
	$(CC) -L/home/jez/code/lib/ $(CFLAGS) -o $@ $+  $(LDFLAGS)

mqtt_channel.o: mqtt_channel.c
	$(CC) $(DFLAGS) $(CFLAGS) -pipe -c $< 


mqttc_sqlite3.o: mqttc_sqlite3.c
	$(CC) $(DFLAGS) $(CFLAGS) -pipe -c $< 

mqtt_parse_config.o: mqtt_parse_config.c
	$(CC) $(DFLAGS) $(CFLAGS) $(DFLAGS) -pipe -c $<  

mqtt_parser_test.o: mqtt_parser_test.c
	$(CC) $(CFLAGS) -pipe -c $<  

clean: mqtt_rpi.o mqtt_rpi
	@rm -v *.o mqtt_rpi mqtt_channel

parser: mqtt_parse_config.o mqtt_parser_test.o
	@ $(CC) $(CFLAGS) $+ -o $@ -lyaml 
	@echo "Executing parser"
	@./parser


test: mqttc parser
	@echo "usage: [test|mqttc|parser|clean]"
	@echo "Building: \""$+"\""
	@echo "Building: \""$?"\""
	@echo "Building: \""$*"\""
