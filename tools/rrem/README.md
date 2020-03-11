<img align="left" src="logo/rrem_256x256.png" width=256>

# rREM
A simple and easy-to-use resource packer and embedder.

<br>
<br>
<br>

## rREM features

 - Multiple resource data types supported (Image, Wave, Text, Raw...)
 - Export **.rres** files, configurable per-resource parameters
 - **Completely portable (single file)**
 - **Powerful command line for batch packaging**
 
## rREM Screenshot

![rREM Command Line](screenshots/rrem_cli.png)
 
## rREM Usage

For fast usage, just drag and drop desired files or directories over the executable, 
they will be automatically processed and packed into a .rres file; a .h file will be
also generated with required details for data loading (generated file IDs and more).

For a more fine-grained usage, rrem allows configuring every resource embedding parameters
individually from its powerful command line interface. For details, just type on command line:

 > rrem.exe --help

## rREM License

rREM is **open source software**. rREM source code is licensed under an unmodified [zlib/libpng license](LICENSE).

rREM is completely free for anyone willing to compile it directly from source. Consider contributing with a small donation to help the author keep working on software for games development.

*Copyright (c) 2015-2020 Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
