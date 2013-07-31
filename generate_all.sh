#!/bin/sh

FILE_PARAMS="boot_args_"
FILE_ARGS=".args"
FILE_PARAM=".param"
RAMDISK_OFFSET=512

CPUS=`cat /proc/cpuinfo | grep processor | awk '{print $3}'`

for CPU in $CPUS
do

  ARGS=`./create_bootargs.sh $CPU`

if [ -n "$ARGS" ] 
then 
echo $ARGS > $FILE_PARAMS$CPU$FILE_ARGS
  for ELEM in $ARGS
  do

     case $ELEM in
     # *memmap=[0-9]*M\$*) echo "$ELEM" ;;
     *memmap=[0-9]*M@*) 
       SIZE=${ELEM#memmap=}
       START=${ELEM#memmap*@}
       echo "FOUND $ELEM ${SIZE%%M*M} ${START%M}" 
     ;;
     *present_mask=*)
       CPUUU=${ELEM#present_mask=}
       echo "FOUND $ELEM $CPUUU"
     ;;
    esac
  done
  BOOT_ADDR=`printf "0x%x\n" $(( ${START%M} << 20 ))`
  RAMDISK_ADDR=`printf "0x%x\n" $(( ( ${START%M} + $RAMDISK_OFFSET ) << 20 ))`

 # echo $ARGS > $FILE_PARAMS$CPU$FILE_ARGS
  echo "vmlinux.elf $FILE_PARAMS$CPU$FILE_ARGS $CPUUU $BOOT_ADDR $RAMDISK_ADDR" > $FILE_PARAMS$CPU$FILE_PARAM
fi
  START_ADDR=""

done




