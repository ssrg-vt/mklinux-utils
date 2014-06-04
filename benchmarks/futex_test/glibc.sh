#! /bin/bash

CPU_COUNT=$1
ROUND=$2
clear
echo "#############Checking Glibc functionalities#######"


if [ "$#" = 0 ]
then
	echo " Usage ./glibc.sh <cpu_core/threads> <round>"
fi



echo " "
echo "#############Run Mutex Tests######################"
echo "+++++CPU SWITCH ++++++++++++++++++"
echo " "
for (( var=0; var<CPU_COUNT; var++ ))
do
    echo "------------Executing tst-mutex2--cpu{$var}-----------"
    /bin/mut 2 0 0 $var
    sleep 0.5
    echo "  "
    echo "------------Executing tst-mutex3--cpu{$var}-----------"
    /bin/mut 3 0 0 $var
    sleep 0.5
    echo " " 
   # echo "------------Executing tst-mutex8--cpu{$var}-----------"
    #/bin/mut 8 0 0 $var
   # sleep 0.5
   # echo " "
done

echo "+++++THREAD COUNT++++++++++++"
echo " "
echo " None for mutex "
echo " "
echo "+++++ THREADS & ROUNDS +++++++"

for (( thread=1; thread<CPU_COUNT; thread++ ))
do
   for (( rd=1; rd<ROUND; rd++ ))
   do
	echo "--------------Executing tst-mutex7------------------"
	/bin/mut 7 $thread $rd
   	sleep 0.5
    done
done

echo "#############Run Barrier Tests######################"

echo "+++++ THREAD COUNT  ++++++++++++"
echo " "
for (( var=0; var<CPU_COUNT; var++ ))
do
	echo "------------Executing tst-bar 4-----------------"
	 /bin/bar 4 $var 0 0
	 sleep 0.5
	 echo "  "
done

echo "+++++ THREADS & ROUNDS +++++++++++"

for (( thread=1; thread<CPU_COUNT; thread++ ))
do
       	for (( rd=1; rd<ROUND; rd++ ))
        do
	echo "--------------Executing tst-barrier3----------------"
 	/bin/bar 2 $thread $rd 0
	done
done


echo "#############Run condition Tests######################"


echo "+++++CPU SWITCH +++++++++++++++++++"
echo " "
for (( var=0; var<CPU_COUNT; var++ ))
do
    echo "------------Executing tst-cond14--cpu{$var}-----------"
    /bin/cond 14 0 0 $var
    sleep 0.5
    echo "  "
    echo "------------Executing tst-cond22--cpu{$var}-----------"
    /bin/cond 22 0 0 $var
    sleep 0.5
    echo " " 
done

echo "+++++THREADS COUNT++++++++++++++++++++"
echo " "
for (( var=0; var<CPU_COUNT; var++ ))
do
	for itr in 2 16 18 #7
	do
		echo "------------Executing tst-cond ${itr} -----------------"
		/bin/cond $itr $var 0 0
		sleep 0.5
		echo "  "
	done
	#echo "------------Executing tst-cond7-----------------"
	#/bin/cond 7 $var 0 0
	#sleep 0.5
	#echo "  "
done


echo "+++++ THREADS & ROUNDS +++++++++++++"

for (( thread=1; thread<CPU_COUNT; thread++ ))
do
	for (( rd=1; rd<ROUND; rd++ ))
	do
	echo "--------------Executing tst-cond 10----------------"
	/bin/cond 10 $thread $rd 0
	sleep 0.5
	echo " "
	done
done


