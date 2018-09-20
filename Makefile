.PHONY: build run kill enter push pull

all:
	g++ json.h RedisClient.hpp ClientService.h stdafx.h NetAdapter.h UdpAdapter.h MainServ.h RedisClient.cpp json.cpp ClientService.cpp MainServ.cpp NetAdapter.cpp UdpAdapter.cpp main.cpp -std=c++11 -lpthread -L./ -lhiredis -lmysqlclient -o tserv	

kill:
	-sudo docker kill ftpd_server
	-sudo docker rm ftpd_server

enter:
	sudo docker exec -it ftpd_server sh -c "export TERM=xterm && bash"

mysql:
	mysql -h127.0.0.1 -u root -pjiangyibo -P4306  gx_wifi_home < gx_wifi_home-20180919.sql
# git commands for quick chaining of make commands
push:
	git push --all
	git push --tags

pull:
	git pull
