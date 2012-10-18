#~/bin/sh

function die {
	echo "ERROR: $*"
	exit 1
}

FIRST=1
if [[ "`hostname`" == "gigi" ]] ; then
	LAST=63
elif [[ "`hostname`" == "found" ]] ; then
	LAST=47
elif [[ "`hostname`" == "rosella" ]] ; then
	LAST=47
else
	die "No configuration for host `hostname`."
fi

[ ! -f ./mklinux_boot.sh ] && die "No ./mklinux_boot, are you in mklinux-utils?"
[ ! -x ./mklinux_boot.sh ] && die "./mklinux_boot is not executable."

for a in `seq $FIRST $END` ; do
	[ ! -f boot_args_${a}.param ] && die "Boot parameters $a are missing."
	./mklinux_boot.sh `cat boot_args_${a}.param`
	sleep 7 # Give the kernel time to boot before continuing.
done
