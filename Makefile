all:
	@echo "run \'make install' to install m1vf-tools"

install:
	g++ -std=c++14 player/main.cpp -o m1play
	cp m1play /usr/local/bin

uninstall:
	-rm -f /usr/local/bin/m1play