.PHONY: build run kill enter push pull

all:
	g++ json.h RedisClient.hpp ClientService.h stdafx.h NetAdapter.h UdpAdapter.h MainServ.h RedisClient.cpp json.cpp ClientService.cpp MainServ.cpp NetAdapter.cpp UdpAdapter.cpp main.cpp -std=c++11 -lpthread -L./ -lhiredis -lmysqlclient -o tserv	
run: kill
	sudo docker run -d --name ftpd_server -p 21:21 -p 30000-30009:30000-30009 -e "PUBLICHOST=localhost" -e "ADDED_FLAGS=-d -d" pure-ftp-demo

kill:
	-sudo docker kill ftpd_server
	-sudo docker rm ftpd_server

enter:
	sudo docker exec -it ftpd_server sh -c "export TERM=xterm && bash"

# git commands for quick chaining of make commands
push:
	git push --all
	git push --tags

pull:
	git pull
