BINUSER=jez
ifdef PNG_GRAPH_PATH
installpath=$(PNG_GRAPH_PATH)
else
installpath="/var/www/pigoda"
endif

all: 
	@echo "Available commands:"
	@echo "install - to install pigoda html files"
	@echo "check -   to check Your installation."
	@echo "Install path=$(installpath)"

/var/www/pigoda/index.html: index.html
	@echo "making index.html"
	#install -m 644 -o root index.html $(installpath)/$<



/var/www/pigoda/custom.html: custom.html
	@echo "installing $<"
	install -m 644 -o root custom.html $(installpath)/$<


install: /var/www/pigoda/index.html
	install -m 644 -o $(BINUSER) index.html $(installpath)/
	install -m 644 -o $(BINUSER) auto.html $(installpath)/
	install -m 644 -o $(BINUSER) custom.html $(installpath)/
	install -m 644 -o $(BINUSER) footer.html $(installpath)/
	install -m 644 -o $(BINUSER) header.html $(installpath)/
	install -m 644 -o $(BINUSER) weekly.html $(installpath)/
	install -m 644 -o $(BINUSER) monthly.html $(installpath)/
	install -m 644 -o $(BINUSER) daily.html $(installpath)/
	install -d -m 755 -o $(BINUSER) $(installpath)/js
	install -m 644 -o $(BINUSER) pigoda.js $(installpath)/js/
	install -m 644 -o $(BINUSER) js.cookie.js $(installpath)/js/
	rsync -a jquery-ui-1.12.1.custom/ $(installpath)/jquery-ui/

check: $(ls *.html)
	@echo "checking $?"
	-diff -c $(PNG_GRAPH_PATH)/$< $<
