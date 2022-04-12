#!/bin/bash

#script that gets one argument - pid of the process
# it prints all the files that are mapped to its memory
# which are found in  /proc/[pid]/maps
pid=$1

#open the mapping file
maps_file=/proc/$pid/maps

#read it line by line
while IFS= read -r line
do
   ## take some action on $line
  echo "$line"
done < < (command)
