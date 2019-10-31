#!/bin/bash

store_ok=0
store_total=0
retrieve_ok=0
retrieve_total=0
delete_ok=0
delete_total=0
client_no=0
connection_ok=0
connection_error=0

while IFS= read -r line
do
	IFS=";"
  	read -ra v <<< "$line"
  	command=${v[1]}
  	if [ "$command" = "STORE" ]; then
		store_ok=$((store_ok+${v[2]}))
		store_total=$((store_total+${v[3]}))
		connection_ok=$((connection_ok+1))
		client_no=$((client_no+1))
	fi
	if [ "$command" = "RETRIEVE" ]; then
		retrieve_ok=$((retrieve_ok+${v[2]}))
		retrieve_total=$((retrieve_total+${v[3]}))
		connection_ok=$((connection_ok+1))
		client_no=$((client_no+1))
	fi
	if [ "$command" = "DELETE" ]; then
		delete_ok=$((delete_ok+${v[2]}))
		delete_total=$((delete_total+${v[3]}))
		connection_ok=$((connection_ok+1))
		client_no=$((client_no+1))
	fi
	if [ "$command" = "CONNERROR" ]; then
		connection_error=$((connection_error+1))
		client_no=$((client_no+1))
	fi
	if [ "$command" = "SERVER" ]; then
		server_pid=${v[0]}
	fi
done < "testout.log"

echo "CLIENTS : " "$client_no"
echo "SUCCESFULL CONNECTIONS : " "$connection_ok" " of " "$client_no"
echo "CONNECTION ERRORS : " "$connection_error"
echo "SUCCESFULL STORE : " "$store_ok" " of " "$store_total"
echo "SUCCESFULL RETRIEVE : " "$retrieve_ok" " of " "$retrieve_total"
echo "SUCCESFULL DELETE : " "$delete_ok" " of " "$delete_total"

kill -SIGUSR1 $server_pid
kill -SIGINT $server_pid