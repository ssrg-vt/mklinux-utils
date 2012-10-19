#~/bin/sh

function die {
	echo "ERROR: $*"
	exit 1
}

SLEEP_TIME=20

FIRST=1
if [[ "`hostname`" == "gigi" ]] ; then
	MAX=63
elif [[ "`hostname`" == "found" ]] ; then
	MAX=47
elif [[ "`hostname`" == "rosella" ]] ; then
	MAX=47
else
	die "No configuration for host `hostname`."
fi

[ $# -eq 0 ] && LAST="$MAX"
[ $# -eq 1 ] && LAST="$1"
[ $# -eq 2 ] && FIRST="$1" && LAST="$2"
[ $# -eq 3 ] && FIRST="$1" && LAST="$2" && SLEEP_TIME="$3"
echo "Will boot kernels $FIRST though $LAST (`seq $FIRST $LAST | wc -l` guest kernels), one every $SLEEP_TIME seconds"

[ $FIRST -eq 0 ] && die "0 is the host kernel."
[ $FIRST -gt $MAX ] && die "Cannot boot kernels higher than $MAX"
[ $LAST -lt $FIRST ] && die "Cannot boot kernels in reverse."

[ ! -f ./mklinux_boot.sh ] && die "No ./mklinux_boot, are you in mklinux-utils?"
[ ! -x ./mklinux_boot.sh ] && die "./mklinux_boot is not executable."

for a in `seq $FIRST $LAST` ; do
	[ ! -f boot_args_${a}.param ] && die "Boot parameters $a are missing."
	./mklinux_boot.sh `cat boot_args_${a}.param`
	sleep $SLEEP_TIME # Give the kernel time to boot before continuing.
done
