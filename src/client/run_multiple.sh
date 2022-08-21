#!/bin/bash

for i in {1..100}
do
  echo $i
  nohup ./c > log.txt 2>&1 &
done
