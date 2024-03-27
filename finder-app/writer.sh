#!/bin/bash

if [ $# -lt 2 ]
then
    echo "Please provide two args: <writer file PATH> <writer string>!"
    exit 1
fi

WRITER_FILE=$1
WRITER_STRING=$2

#echo "Writer file: {$WRITER_FILE}  and Writer string: {$WRITER_STRING}"
FILE_PATH=$(dirname $WRITER_FILE)
FILE_NAME=$(basename $WRITER_FILE)
mkdir -p $FILE_PATH
touch $WRITER_FILE

echo $WRITER_STRING > $WRITER_FILE

exit 0
