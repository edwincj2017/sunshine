#!/bin/sh


make clean;
make CC=arm-linux-gcc;
make install

cd build

rm *.tgz

mv output/samples/* output -f

arm-linux-strip output/libpaho-embed-mqtt3c.so.1.0

tar -czf output_embbed.tgz output
