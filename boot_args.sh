#!/bin/sh

#configuration param
LOG_SERIAL="ttyS0,115200"
ARG_LOG="earlyprintk=$LOG_SERIAL console=$LOG_SERIAL"

#configuration param
LOG_LAPICTIMER="1000000"
ARG_IRQ="acpi_irq_nobalance no_ipi_broadcast lapic_timer=$LOG_LAPICTIMER"

#configuration param
ARG_PCI="pci_dev_flags=0x8086:0x10c9:b,0x102b:0x0532:b,0x1002:0x5a10:b,0x1002:0x4390:b,0x1002:0x4396:b,0x1002:0x4397:b,0x1002:0x4398:b,0x1002:0x4399:b"

ARG_MISC="mklinux debug"

isndigit ()
{
  [ $# -eq 1 ] || return 0
  case $1 in
    *[!0-9]*|"") return 0;;
    *) return 1;;
  esac
}

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

RET=`./mpart > mpart_map`

ARG_PART=`cat mpart_map | grep "present_mask=$1 "`
if [ $? -ne 0 ]
then
  exit 0
fi

echo $ARG_LOG $ARG_IRQ $ARG_PCI $ARG_MISC $ARG_PART
