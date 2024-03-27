#!/bin/bash

if [ $# -lt 2 ] 
then
    echo "Please provide 2 args: <file dir> <search files>"
    exit 1
fi    

FILE_DIR=$1
SEARCH_FILES=$2

echo "File directory: \"${FILE_DIR}\" and search file: \"${SEARCH_FILES}"\"

if [ -d $FILE_DIR ]; then
  #echo "Directory exists."
  Y=$(grep -r $SEARCH_FILES $FILE_DIR/* | wc -l)
  X=$(find $FILE_DIR -name *.txt  | wc -l)
  echo "The number of files are $X and the number of matching lines are $Y"
else
  echo "Directory does not exist."
  exit 1
fi

exit 0