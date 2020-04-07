### WARNING: `rres` and `rrem` are still under development, they are not ready yet... hopefully soon...


# rRES - raylib resource file-format
A simple and easy-to-use file-format to package raylib resources.

rres has been designed to package game assets data into a simple self-contained comprehensive format, easy to read and use, prepared to load data in a fast and efficient way to be directly deployed to memory.

rres has been inspired mainly by [XNB](http://xbox.create.msdn.com/en-US/sample/xnb_format) file-format (used by XNA) but also [RIFF](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format), [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) and [ZIP](https://en.wikipedia.org/wiki/Zip_(file_format)) file-formats.

## Format Design

First design of the format was limited to packaging one resource after another, every resource consisted of one `InfoHeader` followed by a fixed set of four possible parameters and the resource data. Along the .rres file, a .h header file was also generated to map the `resId` with a resource name (usually the original filename of the un-processed data). This model was pretty simple and intuitive but it has some important downsides, like not considering complex pieces of data that could require multiple chunks.

![rres first design](design/rres_file_format_REV0.png)

Second design was way more complex and tried to address first design shortcomings. In this design every resource could consist of multiple chunks of separate data, clearly defined by a set of properties and parameters. Actually, that design is more aligned with RIFF file-format and how most file types are structure, we could understand every resource in the package as a separate file on its own; also, this new design improves packaging possibilities and features.

![rres second design](design/rres_file_format_REV2.png)

## Tools

rREM was a preliminary implementation of a tool to support rres file generation from multiple assets data.

## License

rRES file-format is licensed under MIT license. Check [LICENSE](LICENSE) for further details.

*Copyright (c) 2014-2018 Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
