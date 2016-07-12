#!/bin/sh

# Copyright 2013-2014 Antonio Barbalace, SSRG VT

isndigit ()
{
  [ $# -eq 1 ] || return 0
  case $1 in
    *[!0-9]*|"") return 0;;
    *) return 1;;
  esac
}

# what does this do?
numformat ()
{
  echo $(echo $1 | awk '{ if ($1 < 1024) printf("%d\n", $1); else if ($1 < (1024*1024)) printf("%dk\n", $1/1024); else if ($1 < (1024*1024*1024)) printf("%dM\n", $1/(1024*1024)); else printf("%dG\n", $1/(1024*1024*1024)); }')
}

#configuration params - console
LOG_SERIAL="ttyS0,115200"
ARG_LOG="earlyprintk=$LOG_SERIAL console=$LOG_SERIAL"

#configuration params - ACPI/APIC
LOG_LAPICTIMER="1000000"
ARG_IRQ="acpi_irq_nobalance no_ipi_broadcast=1 lapic_timer=$LOG_LAPICTIMER"

#configuration params - pci devices
#ARG_PCI="pci_dev_flags=0x8086:0x10c9:b,0x102b:0x0532:b,0x1002:0x5a10:b,0x1002:0x4390:b,0x1002:0x4396:b,0x1002:0x4397:b,0x1002:0x4398:b,0x1002:0x4399:b"
_ARG_PCI=`./create_lspci.sh`
if [ $? -ne 0 ]
then
  echo "./create_lspci.sh returned non-zero code and echoed $_ARG_PCI"
  exit 1
fi
ARG_PCI="pci_dev_flags=$_ARG_PCI"

#configuration params - low memory (1MB)
command -v gawk >/dev/null 2>&1 || { echo "gawk not found in the system. please install it and try again" ; exit 1 ; }
TRAMPOLINE_BSP=`dmesg | grep "trampoline BSP"`
if [ -z "$TRAMPOLINE_BSP" ]
then
  echo "not a Popcorn kernel, reboot with a Popcorn kernel"
  exit 1
fi
TRAMPOLINE=`dmesg | sed -n '/trampoline/p' | sed 's/\(at \[[0-9a-fA-F]*\] \)\([0-9a-fA-F]*\) size [0-9]*/\2/' | awk '{print $NF}' | gawk 'BEGIN {i=0x1000000;} { val=strtonum("0x"$1); if (val < i) i=val; } END {printf("%d\n", i);}'`
if [ -z "$TRAMPOLINE" ]
then
  echo "error getting trampoline addresses"
  exit 1
fi
LOW_ADDRESS=65536
LOW_ADDRESS_VAL="$(numformat $LOW_ADDRESS)"
if [ $LOW_ADDRESS -ge $TRAMPOLINE ]
then
  echo "error $LOW_ADDRESS > $TRAMPOLINE"
  exit 1
fi
DISALLOWED_RANGE="$(numformat $(expr $TRAMPOLINE - $LOW_ADDRESS))"
ARG_MEM="memmap=$DISALLOWED_RANGE\$$LOW_ADDRESS_VAL"

#configuration params - others
ARG_MISC="mklinux debug"

###############################################################################

EXPECTED_ARGUMENTS=1
E_BADARGS=65

if [ $# -ne $EXPECTED_ARGUMENTS ]
then
  exit $E_BADARGS
fi

if isndigit $1
then
  exit $E_BADARGS
fi
# is the file mpart_map needed?
./mpart > mpart_map
ARG_PART=`cat mpart_map | grep "present_mask=$1[ -]"`
if [ $? -ne 0 ]
then
  echo "mpart failed"
  exit 1
fi

echo $ARG_LOG $ARG_IRQ $ARG_PCI $ARG_MISC $ARG_PART $ARG_MEM
exit 0
