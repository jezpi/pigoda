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


install: index.html custom_graphs.html
	install -m 644 -o root index.html $(installpath)/
	install -m 644 -o root custom_graphs.html $(installpath)/
	install -m 644 -o root menu.html $(installpath)/
	install -m 644 -o root weekly.html $(installpath)/
	install -m 644 -o root monthly.html $(installpath)/
	install -m 644 -o root daily.html $(installpath)/
	
