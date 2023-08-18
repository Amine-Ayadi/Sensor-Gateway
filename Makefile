zip:
	#zip lab4.zip lib/dplist.c
	zip final.zip connmgr.c sbuffer.c sbuffer.h main.c sensor_db.c sensor_db.h datamgr.c datamgr.h connmgr.h config.h lib/dplist.c lib/dplist.h lib/tcpsock.c lib/tcpsock.h

clean:
	rm -rf libsock.so tcpsock.o
	rm -rf liblist.so dplist.o
tcplib: clean
	gcc -c -fpic lib/tcpsock.c
	gcc -shared -o lib/libsock.so tcpsock.o

dplist: clean
	gcc -c -fpic lib/dplist.c
	gcc -shared -o lib/liblist.so dplist.o

# in lib file (cd lib) but uncomment main()
list: 
	gcc -c -g -Wall -std=c11 -Werror dplist.c -o dplist.o && gcc -o dplist dplist.o && ./dplist 

git: 
	git add . && git commit -m "update recent, log_event commented, log process blocks the code, to fix" && git push --force -u origin main

client:
	gcc -c -g -Wall -std=c11 -Werror sensor_node.c -o sensor_node.o && gcc -o sensor_node sensor_node.o -lm -L./lib -Wl,-rpath=./lib -lsock -llist

client_run: client
	./sensor_node $(ID) 15 1 0.0.0.0 5678

client_run1: client
	./sensor_node $(ID) 21 2 0.0.0.0 5678	

client_run2: client
	./sensor_node $(ID) 37 3 0.0.0.0 5678

client_run1: client
	./sensor_node $(ID) 21 2 0.0.0.0 5678	

datamgr: tcplib dplist
	gcc -D SET_MIN_TEMP=14 -D SET_MAX_TEMP=28 -c -g -Wall -std=c11 -Werror datamgr.c -o datamgr.o && gcc -o datamgr datamgr.o -lm -L./lib -Wl,-rpath=./lib -lsock -llist -lcheck 

datamgr_run: datamgr
	./datamgr

connmgr: tcplib dplist
	gcc -D TIMEOUT=5 -D PORT=5678 -c -g -Wall -std=c11 -Werror connmgr.c -o connmgr.o && gcc -o connmgr connmgr.o -lm -L./lib -Wl,-rpath=./lib -lsock -llist 

connmgr_run: connmgr
	./connmgr

sqlite: tcplib dplist
	gcc -c -g -DDB_NAME=Sensor.db -DTABLE_NAME=SensorData -Wall -std=c11 -Werror sensor_db.c -o sensor_db.o && gcc -o sensor_db sensor_db.o -lsqlite3 -lm -L./lib -Wl,-rpath=./lib -lsock -llist

sqlite_run: sqlite
	./sensor_db

buffer: tcplib dplist
	gcc -c -g -Wall -std=c11 -Werror -lpthread -lsqlite3 -DTIMEOUT=5 -DSET_MIN_TEMP=10 -DDB_NAME=Sensor.db -DTABLE_NAME=SensorData -D TIMEOUT=5 -D PORT=5678 main.c connmgr.c datamgr.c sbuffer.c sensor_db.c -o main.o connmgr.o datamgr.o sbuffer.o sensor_db.o 

lib_buffer: tcplib dplist
	gcc -o main.c connmgr.c datamgr.c sbuffer.c sensor_db.c main.o connmgr.o datamgr.o sbuffer.o sensor_db.o -lm -L./lib -Wl,-rpath=./lib -lsock -llist

# MAIN PROJECT WITHOUT STORE MGR
buf: tcplib dplist
	gcc -g -Wall -std=gnu11 -Werror -pthread main.c datamgr.c sbuffer.c connmgr.c -o main -D TIMEOUT=5 -D SET_MIN_TEMP=14 -D SET_MAX_TEMP=28 -lm -L./lib -Wl,-rpath=./lib -lsock -llist

buf_run: buf
	./main

debug: gw
	clang -fsanitize=thread main.c -o main

gw: tcplib dplist
	gcc -g -Wall -std=gnu11 -Werror -lsqlite3 -lpthread main.c datamgr.c sbuffer.c connmgr.c sensor_db.c -o main -DDB_NAME=Sensor.db -DTABLE_NAME=SensorData -D TIMEOUT=60 -D PORT=5678 -D SET_MIN_TEMP=20 -D SET_MAX_TEMP=28 -lm -L./lib -Wl,-rpath=./lib -lsock -llist -lsqlite3

gw_run: gw
	./main 5678

gdb: gw
	gdb ./main 5678 -tui

mem: gw
	valgrind ./main


# here
gw_parent: tcplib dplist
	gcc -g -O0 -Wall -std=gnu11 -lsqlite3 -lpthread main_parent.c datamgr.c sbuffer.c connmgr.c sensor_db.c -o main_parent -DDB_NAME=Sensor.db -DTABLE_NAME=SensorData -D TIMEOUT=5 -D PORT=5678 -D SET_MIN_TEMP=20 -D SET_MAX_TEMP=28 -lm -L./lib -Wl,-rpath=./lib -lsock -llist -lsqlite3


gw_child: tcplib dplist
	gcc -g -Wall -std=gnu11 -lsqlite3 -lpthread main_child.c datamgr.c sbuffer.c connmgr.c sensor_db.c -o main_child -DDB_NAME=Sensor.db -DTABLE_NAME=SensorData -D TIMEOUT=60 -D PORT=5678 -D SET_MIN_TEMP=20 -D SET_MAX_TEMP=28 -lm -L./lib -Wl,-rpath=./lib -lsock -llist -lsqlite3

all: gw_parent gw_child

run_all: all
	./main_parent 5678 & ./main_child 5678



separate: gw_new
	./main_parent 5678 & ./main_child 5678