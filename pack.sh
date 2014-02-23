#!/bin/bash

#Copy the submission files to the submission dir
#and tar them for autolab

rm myfsck.tar
rm -r submission

mkdir submission && \
cp *.c submission/ && \
cp *.h submission/ && \
cp Makefile submission/ && \
cd submission && \
tar -cvf myfsck.tar *.c *.h Makefile && \
mv myfsck.tar ../ && \
cd ..
