# Sensor-Gateway
This multi-threaded sensor gateway monitors the temperature of a building. It runs a connection manager, "connmgr" which runs a server continuously listening with "epoll" waiting for incoming clients.
The connmgr adds received data to a shared buffer which forwards the data to the "datamgr", a data manager thread that computes a running average and determine if any intervention is necessary. 
The third thread is the store manager, "storemgr", stores the incoming data of every client in an SQLite3 database.  
