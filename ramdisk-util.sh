#!/bin/sh

extract () {
	#img file 
}

compress () {
#move to the initrd directory
#send the file to a tmp file
find . | cpio --create --format='newc' > ../newinitrd
#check for errors
cd ..
gzip newinitrd
#check for existance
mv newinitrd.gz mklinux-utils/ramdisk.img
#remove temp file
}

if [ $(/usr/bin/id -u) -ne 0 ]; then
	echo "Please run this script as root."
	exit 1
fi

# function, img_path, img_file

IMG_FILE=$1
IMG_PATH=$2
echo $IMG_FILE $IMG_PATH

# check img_path existance

# check img_file existence
# check img_file is "file $IMG_FILE | grep newinitrd" AND "file $IMG_FILE | grep gzip"

TMP_FILE=`tempfile`

# check for return error
zcat $IMG_FILE > $TMP_FILE

cd $IMG_PATH
cpio -id < $TMP_FILE
rm $TMP_FILE

