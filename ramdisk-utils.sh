#!/bin/bash

#
# Antonio Barbalace, Popcorn Project, SSRG Virginia Tech, 2015
#

#IMG_PATH=""
IMG_FILE=""
ROOT_PATH=""
#ROOT_FILE=""
VERBOSE=""

TMP_EXT=".gz"

## assume that the extraction directory exists
extract () {
  [ ! -z $VERBOSE ] && echo "extract $IMG_FILE in $ROOT_PATH"
  
  TMP_FILE=`tempfile`
  
  [ ! -z $VERBOSE ] && echo "extract gz to $TMP_FILE"
  cp $IMG_FILE $TMP_FILE$TMP_EXT
  zcat $TMP_FILE$TMP_EXT > $TMP_FILE

  [ ! -z $VERBOSE ] && echo "extract cpio in $ROOT_PATH"  
  cd $ROOT_PATH
  cpio -id < $TMP_FILE
  cd -

  [ ! -z $VERBOSE ] && echo "extract remove temp"  
  rm $TMP_FILE$TMP_EXT $TMP_FILE
}

## remove the img file if already exists
compress () {
  [ ! -z $VERBOSE ] && echo "compress $ROOT_PATH in $IMG_FILE"
  
  TMP_FILE=`tempfile`
  
  [ ! -z $VERBOSE ] && echo "compress to $TMP_FILE"
  cd $ROOT_PATH
  find . | cpio --create --format='newc' > $TMP_FILE
  cd -
  
  [ ! -z $VERBOSE ] && echo "compress to $TMP_FILE$TMP_EXT and move to $IMG_FILE"
  gzip -c $TMP_FILE > $TMP_FILE$TMP_EXT
  mv $TMP_FILE$TMP_EXT $IMG_FILE

  [ ! -z $VERBOSE ] && echo "compress remove temp"  
  #rm $TMP_FILE$TMP_EXT $TMP_FILE
  rm $TMP_FILE
}

parse_config () {
  CONFIG=$1
#  IMG_PATH=`awk -F\= '/IMG_PATH/{print $2}' $CONFIG`
  IMG_FILE=`awk -F\= '/IMG_FILE/{print $2}' $CONFIG`
  ROOT_PATH=`awk -F\= '/ROOT_PATH/{print $2}' $CONFIG`
#  ROOT_FILE=`awk -F\= '/ROOT_FILE/{print $2}' $CONFIG`
}

print_usage () {
  echo "usage:"
  echo "  -a action  Specify which action [c] stands for compress, while [x] for extract."
  echo "  -c config  Specify the config file to fetch to configure the application. Params"
  echo "             for the configuration file are IMG_PATH IMG_FILE ROOT_PATH ROOT_FILE."
  echo "  -f file    File name for compression or extraction."
  echo "  -p path    Directory path for compression or extraction."
  echo "  -v         Verbose mode for debugging. "
}

################################################################
##  main
################################################################

## only root can run the script
if [ $(/usr/bin/id -u) -ne 0 ]; then
        echo "Please run this script as root."
        exit 1
fi

## parse application's arguments
INFILE=""
INPATH=""
TODO=""
CONFIG=""

while [[ $# > 1 ]]; do
key="$1"
case $key in
  -c|--config)
  CONFIG="$2"
  shift
  ;;
  -f|--file)
  INFILE="$2"
  shift
  ;;
  -p|--path)
  INPATH="$2"
  shift
  ;;
  -a|--action)
  TODO="$2"
  shift
  ;;
  -v|--verbose)
  VERBOSE=1
  ;;
  *)
  echo "unknown option $2"
  print_usage
  exit 1
  ;;
esac
shift
done

## verbose is the only option without argument
if [ "$1" == "-v" ] || [ "$1" == "--verbose" ]; then
  VERBOSE=1
fi
#echo "arg file=$INFILE path=$INPATH action=$TODO config=$CONFIG verbose=$VERBOSE"

## load configuration file if it exists
if [ -f "$CONFIG" ]; then
  parse_config $CONFIG
fi
#echo "par file=$IMG_FILE path=$IMG_PATH rfile=$ROOT_FILE rpath=$ROOT_PATH"

## check parameter existence --- note that if the directory doesn't exist we shuold create it
[ ! -z "$INPATH" ] && ROOT_PATH=$INPATH
[ ! -z "$INFILE" ] && IMG_FILE=$INFILE
#echo "par file=$IMG_FILE path=$IMG_PATH rfile=$ROOT_FILE rpath=$ROOT_PATH"

## check the action and based on that configure and run the system
case $TODO in
  c|compress)
  if [ -d "$ROOT_PATH" ] && [ ! -d "$IMG_FILE" ]; then
    [ -f "$IMG_FILE" ] && rm "$IMG_FILE"
    compress
    #[ ! -z $VERBOSE ] && echo "compression verbose" || echo "compression not verbose"
  else
    echo "compress, something went wrong path=$ROOT_PATH file=$IMG_FILE"
  fi
  ;;
  x|extract)
  if [ -f "$IMG_FILE" ] && [ ! -f "$ROOT_PATH" ]; then
    [ ! -d "$ROOT_PATH" ] && mkdir "$ROOT_PATH"
    extract
    #[ ! -z $VERBOSE ] && echo "extraction verbose" || echo "extraction not verbose"
  else
    echo "extract, something went wrong path=$ROOT_PATH file=$IMG_FILE"
  fi
  ;;
  *)
  echo "action not supported $TODO or no arguments specified"
  print_usage
  exit 1
  ;;
esac

exit 0
