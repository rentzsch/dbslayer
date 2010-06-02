#!/bin/sh
cd "`dirname \"$0\"`"
./server/dbslayer -c empty_my.cnf -u root -h localhost -s dbslayer
