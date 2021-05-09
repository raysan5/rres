<img align="left" src="https://github.com/raysan5/rres/blob/master/logo/rres_256x256.png" width=256>

<br>

**rres is a simple and easy-to-use file-format to package resources**

`rres` has been designed to package game assets data into a simple self-contained comprehensive format, easy to read and write, prepared to load data in a fast and efficient way.

`rres` is inspired by [XNB](http://xbox.create.msdn.com/en-US/sample/xnb_format) file-format (used by XNA/MonoGame), [RIFF](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format), [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) and [ZIP](https://en.wikipedia.org/wiki/Zip_(file_format)) file-formats.

<br>
<br>

## rres format design

First design of the format was limited to packaging one resource after another, every resource consisted of one `InfoHeader` followed by a fixed set of four possible parameters and the resource data. Along the .rres file, a .h header file was also generated to map the `resId` with a resource name (usually the original filename of the un-processed data). This model was pretty simple and intuitive but it has some important downsides: not considering complex pieces of data that could require multiple chunks.

Second design was way more complex and tried to address first design shortcomings. In this design every resource could include multiple chunks of separate data, clearly defined by a set of properties and parameters. Actually, that design similar to RIFF file-format and how most file types are structure: every resource in the package could be threatened as a separate file on its own. Another addition of second design was some improvements on packaging options and features.

Third design...

Fourth design...

## Tools

_IN PROGRESS_

## License

rRES file-format is licensed under MIT license. Check [LICENSE](LICENSE) for further details.

*Copyright (c) 2014-2021 Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
