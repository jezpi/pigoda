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
	install -m 644 -o root index.html $(installpath)/$<

install: /var/www/pigoda/index.html /var/www/pigoda/custom.html
	install -m 644 -o root index.html $(installpath)/
	install -m 644 -o root auto.html $(installpath)/
	install -m 644 -o root custom.html $(installpath)/
	install -m 644 -o root footer.html $(installpath)/
	install -m 644 -o root header.html $(installpath)/
	install -m 644 -o root weekly.html $(installpath)/
	install -m 644 -o root monthly.html $(installpath)/
	install -m 644 -o root daily.html $(installpath)/


check: $(ls *.html)
	@echo "checking $?"
	-diff -c $(PNG_GRAPH_PATH)/$< $<
