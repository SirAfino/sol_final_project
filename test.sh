#!/bin/bash 

pids=[]
./server &
server_pid=$!

#store test
for i in {1..50}
do
	username="user$i"
	./client $username 1 >> ./testout.log &
	pids[${i}]=$!
done
for pid in ${pids[*]}; do
	if [ $pid != [] ]; then
		wait $pid
	fi
done

#retrieve and delete test
pids=[]
for i in {1..30}
do
	username="user$i"
	./client $username 2 >> ./testout.log &
	pids[${i}]=$!
done
for i in {31..50}
do
	username="user$i"
	./client $username 3 >> ./testout.log &
	pids[${i}]=$!
done
for pid in ${pids[*]}; do
	if [ $pid != [] ]; then
		wait $pid
	fi
done

echo "$server_pid" ";SERVER" >> testout.log