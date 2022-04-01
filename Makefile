all:
	@echo "run \'make install' to install m1vf-tools"

install:
	g++ -std=c++14 -o m1play decoder/m1vf.cpp player/main.cpp -Idecoder
	cp m1play /usr/local/bin

uninstall:
	-rm -f /usr/local/bin/m1play