#!/bin/sh

CURR_EXEC="/home/antoniob/mklinux/mklinux-gmake"
JOBS_EXEC=4

make -f $CURR_EXEC/Makefile -j $JOBS_EXEC &
echo "make pid $!"

exit

CURR_DIR=`pwd`
TARGET_DIR="/proc"

cd $TARGET_DIR
DIR_LIST=`ls -d *`
cd $CURR_DIR

WARN=0
ERROR=0

for ENTRIES in $DIR_LIST
do
  if [[ $ENTRIES =~ ^[0-9]+$ ]]
  then
    EXECUTABLE="$TARGET_DIR/$ENTRIES"
    #echo $EXECUTABLE
    if [ -f "$EXECUTABLE/comm" ]
    then
      NAME=`cat $EXECUTABLE/comm`
    else
      #echo "ERROR $EXECUTABLE/comm file doesn't exist"
      continue
    fi

    if [[ $NAME = "" ]]
    then
      echo "ERROR $NAME $EXECUTABLE"
      continue
    fi

    DATA=`./rd "$EXECUTABLE"`
    if [[ ${DATA:0:4} = "WARN" ]]
    then
      WARN=`expr $WARN + 1`
    elif [[ ${DATA:0:5} = "ERROR" ]]
    then
      ERROR=`expr $ERROR + 1`
      echo "ERROR $NAME:$ENTRIES $DATA"
    else
      echo "$NAME:$ENTRIES $DATA"
    fi
  fi
done

#echo "warn=$WARN error=$ERROR"
