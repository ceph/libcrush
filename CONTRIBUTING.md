# Linux kernel

Some __libcrush__ files are copy/pasted into the Linux kernel:
crush.[ch], crush_ln_table.h, hash.[ch] and mapper.[ch]. This imposes
additional restrictions on the implementation and coding style.

- do not use floating point
- use the functions and constants from crush_compat.h where relevant
- try to follow the [linux kernel coding style](https://github.com/torvalds/linux/blob/1dc4bbf0b268246f6202c761016735933b6f0b99/Documentation/process/coding-style.rst)
