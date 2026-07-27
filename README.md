# Central repository for drivers.
In this repository, we put some open-source drivers for E-comOS' because some service need output, by VGA, but Ring 3's service cannot write VGA's memory address. 
For that we develop this repository's first driver called [eightVGA](vga/eightvga.c). 
## How to Join Up
We support maling list, aren't GitHub Pull Reuqest, please send your patch to <e-comos-drivers@googlegroups.com>.
## Develops Rules
### Coding Style.
About coding style please see [E-comOS Coding Style Guide](https://github.com/e-comos/e-comos/blob/main/docs/coding-style.md) to learn more. 
However, if you don't follow these rules, may be your patch(es) will be rejected. For more infos, see [what should you do before you send your patch(es)](https://github.com/e-comos/e-comos/blob/main/docs/before-send-your-patch.md).  
### How to Build
You can use the make to get a 'drivers.bin' to use. 
And we think the good way is driver should be **a** file, but if it have a lot of files, you should put a Makefile in your drivers root folder. 
To compile **a** file, you just need 'gcc file.c -o file.o -Wall -Werror -I...'. After that you just need use the objcopy to convert it to a binary file. 
## Drivers
We hope your driver is follow the GPLv3 or later. But if you cannot, your driver(s)' license should be compatible with GPLv3. 
For blobs, you have to set a stub to our operating system.