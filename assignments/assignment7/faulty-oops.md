# Assignement 7

## Kernel Oops error:

```
echo “hello_world” > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b3a000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: faulty(O) scull(O) hello(O)
CPU: 0 PID: 151 Comm: sh Tainted: G           O       6.1.44 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc8/0x390
sp : ffffffc008dfbd20
x29: ffffffc008dfbd80 x28: ffffff8001b9cf80 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000000000012 x22: 0000000000000012 x21: ffffffc008dfbdc0
x20: 000000558f853990 x19: ffffff8001bcf500 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc00078c000 x3 : ffffffc008dfbdc0
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 0000000000000000 ]---
```

## Oops analysis: 
This is a kernel panic (Oops) error caused by a NULL pointer dereference in the faulty kernel module when writing to /dev/faulty.

1. Faulty module issue:
    -   The function faulty_write+0x10/0x20 [faulty] is responsible for handling writes to /dev/faulty.
    -   The faulty_write() function is trying to dereference a NULL pointer, causing a level 1 translation fault.

2.  Key indicators from the error log:
    -   Virtual address 0000000000000000 → NULL pointer dereference.
        ESR = 0x96000045 → Data Abort due to a bad virtual address.
        FSC = 0x05 (level 1 translation fault) → Memory translation failure, likely due to a NULL pointer access.
        Tainted Kernel (Tainted: G O) → The presence of external modules (faulty, scull, hello) that are not part of the mainline kernel.

3.  Call trace:
    -   The issue originates from faulty_write(), which is likely trying to dereference an invalid pointer (NULL).
    -   The crash happens when the shell (sh) tries to write "hello_world" to /dev/faulty.