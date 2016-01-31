#!/bin/sh
kernel="vmlinuz-3.2.14ft-merged+"
initrd="initrd.img-3.2.14ft-merged+"
params="root=UUID=6bd7ec5c-96ec-4c5d-9def-576127193147 ro console=tty0 console=ttyS0,115200 earlyprintk=ttyS0,115200 acpi_irq_nobalance no_ipi_broadcast=1 debug vty_offset=0x1fac000000 present_mask=0-31 mem=66176M norandmaps pci_dev_flags=0x8086:0x10c9:0.1:b"
kexec -l "/boot/${kernel}" --append="${params}" --initrd="/boot/${initrd}"
kexec -e
