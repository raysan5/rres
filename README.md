<img align="left" src="https://github.com/raysan5/rres/blob/master/logo/rres_256x256.png" width=256>

<br>

**rres is a simple and easy-to-use file-format to package resources**

`rres` has been designed to package game assets (images, fonts, text, audio, models...) into a simple self-contained comprehensive format, easy to read and write, designed to load data in a fast and efficient way.

`rres` is inspired by [XNB](http://xbox.create.msdn.com/en-US/sample/xnb_format) file-format (used by XNA/MonoGame), [RIFF](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format), [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) and [ZIP](https://en.wikipedia.org/wiki/Zip_(file_format)) file-formats.

<br>
<br>


# Index

1. [Design Goals](#design-goals)
2. [Usage Benefits](#usage-benefits)
3. [Design History](#design-history)
4. [File Structure](#file-structure)
5. [File Header: `rresFileHeader`](#file-header-rresfileheader)
6. [Resource Chunk: `rresResourceChunk`](#resource-chunk-rresresourcechunk)
    1. [Resource Info Header: `rresResourceInfoHeader`](#resource-info-header-rresresourceinfoheader)
    2. [Resource Data Chunk: `rresResourceDataChunk`](#resource-data-chunk-rresresourcedatachunk)
    3. [Resource Data Type: `rresResourceDataType`](#resource-data-type-rresresourcedatatype)
    4. [Resource Chunk: Central Directory: `rresCentralDir`](#resource-chunk-central-directory-rrescentraldir)
7. [Custom Engine Implementation](#custom-engine-implementation)
8. [License](#license)

------------------------------------------

## Design Goals

- **Simplicity**: `rres` structure is simple, one file header and multiple resources one after the other. Still, every resource has a small (32 bytes) resource info header with multiple options available.

- **Ease-of-use**: `rres` library to read `.rres` files is small with only the minimum required functions to read resource data from it's id. It only uses a small set of functions from the standard C library. An `rrespacker` tool with GUI/CLI is provided to easily create and view `.rres` files.

- **Flexibility**: `rres` format support any kind of input file, if data is not classified as a basic data type it can be just packed as raw file data.

- **Portability**: `rres` file format is not tied to an specific engine, the base library just reads the resource data as packed and that data can be mapped to any engine structures. An usage example has been provided with the auxiliar library `rres-raylib.h` that maps rres data to raylib structures.

- **Security**: `rres` support per-resource compression and encryption if required. Still, despite the support provided by the file format is up to the user to implement it in the `rres` packer tool and the engine mapping library. `rres` does not force any specific compression/encryption algorithm by design

- **Extensibility**: `rres` file format is extensible, supporting new data types, new features and custom data if required.

- **Free and open source**: `rres` is an open spec and a free and open source library. It can be used with the provided tool (`rrespacker`) or a custom packer/loader can be implemented for **any engine**. The format description can also be used as a learning material for anyone willing to create its own data packaging file format.

## Usage Benefits

There are some important reasons to package game assets data into a format like `rres`, here some good reasons to do it.

 - **Organization**: Avoid thousands of files distributed in hundreds of directories in the final game build. All the game files data could be organized in a single or a few `.rres` files. It keeps the game directory clean and well structured.

 - **Performance**: Keeping all required files for a level/map into a single `.rres` file reduces the need of file handles to access multiple files; file handles are an OS resource and the cost of opening a file is not insignificant, it may cause delays in the game. Data access times inside the `.rres` file are also important and depending how the files are organized it could also improve the loading times.

 - **Security**: Avoid exposing all the game assets directly to the user for easy copy or modification. Data packed into a `.rres` file will be more difficult to extract and modify for most users; protect your game copyrighted assets. `rres` also supports per-resource data compression and encryption, adding an extra level of security when required. 

 - **Downloading times**: If data needs to be accessed from a server and downloaded, the assets packed into a single or a few `.rres` files improves download times, in comparison to downloading each file individually.

## Design History

`rres` has been in development **since 2014**. I started this project with the aim to create a packaging file-format similar to [XNB](http://xbox.create.msdn.com/en-US/sample/xnb_format) for raylib. In the last **8 years** the project has suffered multiple redesigns and improvements along a file-formats learning process. In that time I implemented loaders/writers for **+20 different file formats** and also created **+12 custom file formats** for multiple [raylibtech custom tools](https://raylibtech.itch.io/).

`rres` file-format has gone through _at least_ **4 big redesigns**:

 - **First design** of the format was limited to packaging one resource after another, every resource consisted of one _resource info header_ followed by a fixed set of four possible parameters and the resource data. Along the `.rres` file, a `.h` header file was generated mapping with defines the `rresId` with a resource filename (usually the original filename of the un-processed data). This model was pretty simple and intuitive but it has some important downsides: it did not consider complex pieces of data that could require multiple chunks and there was no way to extract/retrieve original source files (or similar ones).

 - **Second design** was way more complex and tried to address first design shortcomings. In second design every resource could include multiple chunks of data in a kind of tree structure. Actually, that design was similar to [RIFF](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format) file-format: every chunk could contain additional chunks, each one with its header and data. Some additional resource options were also added but the format become quite complex to understand and manage. Finally the testing implementation was discarded and a simpler alternative was investigated.

 - **Third design** was a return to first design: simplicity but keeping some of the options for the individual resources. The problem of multiple resources generated from a single input file was solved using a simple offset field in the _resource info header_ pointing to next linked resource when required. Resources were loaded as an array of resource chunks. An optional Central Directory resource chunk was added to keep references for the input files. Format was good but it still required implementation and further investigation, it needed to be engine-agnostic and it was still dependant on raylib structures and functionality.

 - **Fourth design** has been done along the implementation, almost all structures and fields have been reviewed and renamed for consistency and simplicity. A separare library has been created (`rres-raylib.h`) to map the loaded resources data into a custom library/engine data types, any depndency to raylib has been removed, making it a completely engine-agnostic file-format. Several usage examples for raylib has been implemented to illustrate the library-engine connection of `rres` and multiple types of resource loading have been implemented. `rrespacker` tool has been created from scratch to generate `rres` files, it supports a nive GUI interface with drag and drop support but also a powerful command-line for batch processing. Compression and encryption implementation has been moved to the user library implementation to be aligned with the packaging tool and keep the `rres` file-format cleaner and simpler.

It's been an **8 years project**, working on it on-and-off, with many redesigns and revisions but I'm personally very happy with the final result. `rres` is a resource packaging file-format at the level of any professional engine package format _BUT free and open-source_, available to any game developer that wants to use it, implement it or create a custom version.

## File Structure

rres file format consists of a file header (`rresFileHeader`) followed by a number of resource chunks (`rresResourceChunk`). Every resource chunk has a resource info header (`rresResourceInfoHeader`) that includes a `FOURCC` data type code and resource data information. The resource data (`rresResourceDataChunk`) contains a small set of properties to identify data, depending on the type and could contain some additional data at the end.

![rres v1.0](https://raw.githubusercontent.com/raysan5/rres/master/design/rres_file_format_REV5.png)

_Fig 01. rres v1.0 file structure._

_NOTE: rresResourceChunk(s) are usually generated from input files. It's important to note that resources could not be mapped to files 1:1, one input file could generate multiple resource chunks. For example, a .ttf input file could generate an image resource chunk (`RRES_DATA_IMAGE` type) plus a font glyph info resource chunk (`RRES_DATA_GLYPH_INFO` type)._


```c
rresFileHeader               (16 bytes)
    Signature Id              (4 bytes)     // File signature id: 'rres'
    Version                   (2 bytes)     // Format version
    Resource Count            (2 bytes)     // Number of resource chunks contained
    CD Offset                 (4 bytes)     // Central Directory offset (if available)
    Reserved                  (4 bytes)     // <reserved>

rresResourceChunk[]
{
    rresResourceInfoHeader   (32 bytes)
        Type                  (4 bytes)     // Resource type (FourCC)
        Id                    (4 bytes)     // Resource identifier (CRC32 filename hash or custom)
        Compressor            (1 byte)      // Data compression algorithm
        Cipher                (1 byte)      // Data encryption algorithm
        Flags                 (2 bytes)     // Data flags (if required)
        Packed data Size      (4 bytes)     // Packed data size (compressed/encrypted + custom data appended)
        Base data Size        (4 bytes)     // Base data size (uncompressed/decrypted)
        Next Offset           (4 bytes)     // Next resource chunk offset (if required)
        Reserved              (4 bytes)     // <reserved>
        CRC32                 (4 bytes)     // Resource Data Chunk CRC32
                             
    rresResourceDataChunk     (n bytes)     // Packed data
        Property Count        (4 bytes)     // Number of properties contained
        Properties[]          (4*i bytes)   // Resource data required properties, depend on Type
        Data                  (m bytes)     // Resource data
}
```

## File Header: `rresFileHeader`

The following C struct defines the `rresFileHeader`:

```c
// rres file header (16 bytes)
typedef struct rresFileHeader {
    unsigned char id[4];            // File identifier: rres
    unsigned short version;         // File version: 100 for version 1.0
    unsigned short chunkCount;      // Number of resource chunks in the file (MAX: 65535)
    unsigned int cdOffset;          // Central Directory offset in file (0 if not available)
    unsigned int reserved;          // <reserved>
} rresFileHeader;
```

| Field | Description |
| :---: | :---------- |
| `id` | File signature identifier, it must be the four characters: `r`,`r`,`e`,`s`. | 
| `version` | Defines the version and subversion of the format. |
| `chunkCount`| Number of resource chunks present in the file. Note that it could be greater than the number of input files processed. |
| `cdOffset` | Central Directory absolute offset within the file, note that `CDIR` is just another resource chunk type, **Central Directory can be present in the file or not**. It's recommended to be placed as the last chunk in the file if a custom `rres` packer is implemented. Check `rresCentralDir` section for more details.
| `reserved` | This field is reserved for future additions if required. |

_Table 01. `rresFileHeader` fields description and details_

Considerations:

 - `rres` files are limited by design to a **maximum of 65535 resource chunks**, in case more resources need to be packed is recommended to create multiple `rres` files.
 - `rres` files use 32 bit offsets to address the different resource chunks, consequently, **no more than ~4GB of data can be addressed**, please keep the `rres` files smaller than **4GB**. In case of more space required to package resources, create multiple `rres` files.

## Resource Chunk: `rresResourceChunk`

`rres` file contains a number of resource chunks. Every resource chunk represents a self-contained pack of data. Resource chunks are generated from input files on `rres` file creation by the `rres` packer tool; depending on the file extension, the `rres` packer tool extracts the required data from the file and generates one or more resource chunks. For example, for an image file, a resource chunk of `type` `RRES_DATA_IMAGE` is generated containing only the pixel data of the image and the required properties to read that data back from the resource file.

It's important to note that one input file could generate several resource chunks when the `rres` file is created. For example, a `.ttf` input file could generate an `RRES_DATA_IMAGE` resource chunk plus a `RRES_DATA_GLYPH_INFO` resource chunk; it's also possible to just pack the file as a plain `RRES_DATA_RAW` resource chunk type, in that case the input file is not processed, just packed as raw data.

On `rres` creation, `rres` packer could create an additional resource chunk of type `RRES_DATA_DIRECTORY` containing data about the processed input files. It could be useful in some cases, for example to relate the input filename directly to the generated resource(s) id and also to extract the data in a similar file structure to the original input one.

Every resource chunk is divided in two part: `rresResourceInfoHeader` + `rresResourceData`.

### Resource Info Header: `rresResourceInfoHeader`

The following C struct defines the `rresResourceInfoHeader`:

```c
// rres resource chunk info header (32 bytes)
typedef struct rresResourceInfoHeader {
    unsigned char type[4];          // Resource chunk type (FourCC)
    unsigned int id;                // Resource chunk identifier (generated from filename CRC32 hash)
    unsigned char compType;         // Data compression algorithm
    unsigned char cipherType;       // Data encription algorithm
    unsigned short flags;           // Data flags (if required)
    unsigned int packedSize;        // Data chunk size (compressed/encrypted + custom data appended)
    unsigned int baseSize;          // Data base size (uncompressed/unencrypted)
    unsigned int nextOffset;        // Next resource chunk global offset (if resource has multiple chunks)
    unsigned int reserved;          // <reserved>
    unsigned int crc32;             // Data chunk CRC32 (propCount + props[] + data)
} rresResourceInfoHeader;
```

| Field | Description |
| :---: | :---------- |
| `type` | A [`FourCC`](https://en.wikipedia.org/wiki/FourCC) code and identifies the type of resource data contained in the `rresResourceDataChunk`. Enum `rresResourceDataType` defines several data types, new ones could be added if requried. |
| `id` | A global resource identifier, it's generated from input filename using a CRC32 hash and it's not unique. One input file can generate multiple resource chunks, all the generated chunks share the same identifier and they are loaded together when the resource is loaded. For example, a input .ttf could generate two resource chunks (`RRES_DATA_IMAGE` + `RRES_DATA_GLYPH_INFO`) with same identifier that will be loaded together when their identifier is requested. It's up to the user to decide what to do with loaded data. |
| `compType` | Defines the compression algorithm used for the resource chunk data. Compression depends on the middle library between rres and the engine, `rres.h` just defines some useful algorithm values to be used in case of implementing compression. Compression should always be applied before encryption and it compresses the full `rresResourceData` (`Property Count` + `Properties[]` + `Data`). If no data encryption is applied, `packedSize` defines the size of compressed data. |
| `cipherType` | Defines the encryption algorithm used for the resource chunk data. Like compression, encryption depends on the middle library between rres and the engine, `rres.h` just defines some useful algorithm values to be used in case of implementing encryption. Encryption should be applied after compression. Depending on the encryption algorithm and encryption mode it could require some extra piece of data to be attached to the resource data (i.e encryption MAC), this is implementation dependant and the rres packer tool / rres middle library for the engines are the responsible to manage that extra data. It's recommended to be just appended to resource data and considered on `packedSize`. |
| `flags` | Reserved for additional flags, in case they are required by the implementation. |
| `packedSize` | Defines the packed size (compressed/encrypted + additional user data) of `rresResourceDataChunk`. Packaged data could contain appended user data at the end, after compressed/encrypted data, for example the nonce/MAC for the encrypted data, but it is implementation dependant, managed by the `rres` packer tool and the user library to load the chunks data into target engine structures. |
| `baseSize` | Defines the base size (uncompressed/unencrypted) of `rresResourceDataChunk`. |
| `nextOffset` | Defines the global file position address for the next _related_ resource chunk, it's useful for input files that generate multiple resources, like fonts or meshes. |
| `reserved` | This field is reserved for future additions if required. |
| `crc32` | Calculated over the full `rresResourceData` chunk (`packedSize`) and it's intended to detect data corruption errors. |

_Table 02. `rresResourceInfoHeader` fields description and details_

### Resource Data Chunk: `rresResourceDataChunk`

`rresResourceDataChunk` contains the following data:

 - `Property Count`: Number of properties contained, depends on resource `type`
 - `Properties[]`: Resource data required properties, depend on resource `type`
 - `Data`: Resource data, depend on resource `type`

_NOTE: rresResourceDataChunk could contain additional user data, in those cases additional data size must be considered in `packedSize`._
 
### Resource Data Type: `rresResourceDataType`

The resource `type` specified in the `rresResourceInfoHeader` defines the type of data and the number of properties contained in the resource chunk.

Here it is the currently defined data types. Please note that some input files could generate multiple resource chunks of multiple types. Additional resource types could be added if required.

```c
// rres resource chunk data type
// NOTE 1: Data type determines the properties and the data included in every chunk
// NOTE 2: This enum defines the basic resource data types, some input files could generate multiple resource chunks
typedef enum rresResourceDataType {

    RRES_DATA_NULL         = 0,     // FourCC: NULL - Reserved for empty chunks, no props/data
    RRES_DATA_RAW          = 1,     // FourCC: RAWD - Raw file data, input file is not processed, just packed as is
    RRES_DATA_TEXT         = 2,     // FourCC: TEXT - Text file data, byte data extracted from text file
    RRES_DATA_IMAGE        = 3,     // FourCC: IMGE - Image file data, pixel data extracted from image file
    RRES_DATA_WAVE         = 4,     // FourCC: WAVE - Audio file data, samples data extracted from audio file
    RRES_DATA_VERTEX       = 5,     // FourCC: VRTX - Vertex file data, extracted from a mesh file
    RRES_DATA_GLYPH_INFO   = 6,     // FourCC: FNTG - Font glyphs info, generated from an input font file
    RRES_DATA_LINK         = 99,    // FourCC: LINK - External linked file, filepath as provided on file input
    RRES_DATA_DIRECTORY    = 100,   // FourCC: CDIR - Central directory for input files relation to resource chunks
    
    // TODO: Add additional data types if required
    
} rresResourceDataType;
```

The currently defined data `types` consist of the following properties and data:

| resource type      |  FourCC  |  propsCount  |    props     |      data        |
| :----------------: | :------: | :----------: | :----------- | :--------------: |
| `RRES_DATA_NULL`   |  `NULL`  |      0       |      -       |        -         |
| `RRES_DATA_RAW`    |  `RAWD`  |      1       | `props[0]`:size  |  raw file bytes  |
| `RRES_DATA_TEXT`   |  `TEXT`  |      4       | `props[0]`:size<br>`props[1]`:`rresTextEncoding`<br>`props[2]`:`rresCodeLang`<br>`props[3]`:cultureCode  |  text data   |
| `RRES_DATA_IMAGE`  |  `IMGE`  |      4       | `props[0]`:width<br>`props[1]`:height<br>`props[2]`:`rresPixelFormat`<br>`props[3]`:mipmaps |  pixel data         |
| `RRES_DATA_WAVE`   |  `WAVE`  |      4       | `props[0]`:frameCount<br>`props[1]`:sampleRate<br>`props[2]`:sampleSize<br>`props[3]`:channels | audio samples data |
| `RRES_DATA_VERTEX` |  `VRTX`  |      4       | `props[0]`:vertexCount<br>`props[1]`:`rresVertexAttribute`<br>`props[2]`:componentCount<br>`props[3]`:`rresVertexFormat` | vertex data |
| `RRES_DATA_GLYPH_INFO`| `FNTG`|      4       | `props[0]`:baseSize<br>`props[1]`:glyphCount<br>`props[2]`:glyphPadding<br>`props[3]`:`rresFontStyle` | `rresFontGlyphInfo[0..glyphCount]` |
| `RRES_DATA_LINK`   |  `LINK`  |      1       | `props[0]`:size       | filepath data |
| `RRES_DATA_DIRECTORY` | `CDIR`|      1       | `props[0]`:entryCount | `rresDirEntry[0..entryCount]` |

_Table 03. `rresResourceDataType` defined values and details_

`rres.h` defines the following `enums` for convenience to assign some of the properties:

 - `rresTextEncoding`: Defines several possible text encodings, default value is 0 (UTF-8)
 - `rresCodeLang`: Defines several programming languages, useful in case of embedding code files or scripts
 - `rresPixelFormat`: Defines multiple pixel format values for image pixel data
 - `rresVertexAttribute`: Defines several vertex attibute types for convenience
 - `rresVertexFormat`: Defines several data formats for vertex data
 - `rresFontStyle`: Defines several font styles (Regular, Bold, Italic...), default value is 0 (`RRES_FONT_STYLE_DEFAULT`) 

### Resource Chunk: Central Directory: `rresCentralDir`

The `Central Directory` resource chunk is a special chunk that **could be present or not** in the `rres` file, it stores information about the input files processed to generate the multiple resource chunks and could be useful to:

 - **Reference the resources by its original filename.** This is very useful in terms of implementation to minimize the required code changes if `rres` packaging is done over an existing project or at the end of a project development, if all files loading has been done using directly the filenames. 

 - **Extract some of the resources to a similar input file.** It will be only possible if the input file has not been processed in a destructive way. For example, if  a.ttf file has been processed to generate a glyphs image atlas + glyphs data info, it won't be possible to retrieve the original .ttf file.

`rres` provides some helpful structures to deal with `Central Directory` resource chunk data:

```c
// rres central directory entry
typedef struct rresDirEntry {
    unsigned int id;                // Resource id
    unsigned int offset;            // Resource global offset in file
    unsigned int reserved;          // reserved
    unsigned int fileNameSize;      // Resource fileName size (NULL terminator and 4-byte alignment padding considered)
    char fileName[RRES_MAX_CDIR_FILENAME_LENGTH];  // Resource original fileName (NULL terminated and padded to 4-byte alignment)
} rresDirEntry;

// rres central directory
// NOTE: This data represents the rresResourceDataChunk
typedef struct rresCentralDir {
    unsigned int count;             // Central directory entries count
    rresDirEntry *entries;          // Central directory entries
} rresCentralDir;
```

_NOTE: Central Directory filename entries are aligned to 4-byte padding to improve file-access times._ 
 
`rres.h` provides a function to load `Central Directory` from the `rres` file when available: **`rresLoadCentralDirectory()`** and also a function to get a resource identifiers from its original filename: **`rresGetIdFromFileName()`**.

In case a `rres` file is generated with no `Central Directory`, a secondary header file (`.h`) should be provided with the id references for all resources, to be used in user code.

## Custom Engine Implementation

`rres` is designed as an **engine-agnostic file format**, processed data is treated as generic data, common to any game engine. Developers can implement a custom abstraction layers to **map `rres` generic data to their own engines data structures** and also **custom `rres` packaging tools**.

The following diagram shows a sample implementation of `rres` for [`raylib`](https://github.com/raysan5/raylib) library.

![rres v1.0](https://raw.githubusercontent.com/raysan5/rres/master/design/rres_implementation.png)

_Fig 02. rres sample implementation: custom engine lib and tool._

`rres` implementation consist of several pieces:

 - [Base library: `rres.h`](#base-library-rresh)
 - [Engine mapping library: `rres-raylib.h`](#engine-mapping-library-rres-raylibh)
 - [Packaging tool: `rrespacker`](#packaging-tool-rrespacker)

### Base library: `rres.h`

Base `rres` library is in charge of reading `rres` files resource chunks into generic resource structures, returned to the user. In our implementation the user exposed resource structures (`rresResourceChunk`, `rresResource`) are different than the ones used internally to process the `rres` file data (`rresFileHeader`, `rresResourceInfoHeader`). This design decision is due to the returned data for the user consist of a combination of the resource data info and resource data, and user does not need to have all the information after being properly processed. Our implementation does not include resource file writing, that functionality has been implemented directly in `rrespacker` tool.

`rresResource` is provided by `rres.h` along functions to load it and contains an array of `rresResourceChunk`, defined as following:

```c
// rres resource chunk
typedef struct rresResourceChunk {
    unsigned int type;              // Resource chunk data type
    unsigned short compType;        // Resource compression algorithm
    unsigned short cipherType;      // Resource cipher algorythm
    unsigned int packedSize;        // Packed data size (including props, compressed and/or encripted + additional data appended)
    unsigned int baseSize;          // Base data size (including propCount, props and uncompressed/decrypted data)
    unsigned int propCount;         // Resource chunk properties count
    int *props;                     // Resource chunk properties
    void *data;                     // Resource chunk data
} rresResourceChunk;

// rres resource
typedef struct rresResource {
    unsigned int count;             // Resource chunks count
    rresResourceChunk *chunks;      // Resource chunks
} rresResource;
```

A `rresResource` could be loaded from the `.rres` file with the provided function: **`rresLoadResource()`**.

After data has been copied to the destination structure it can be unloaded with function: **`rresUnloadResource()`**.

### Engine mapping library: `rres-raylib.h`

The mapping library includes `rres.h` and provides functionality to load the generic resource data loaded from the `rres` file into `raylib` structures. The API provided is simple and intuitive, following `raylib` conventions:

```c
RLAPI void *LoadDataFromResource(rresResource rres, int *size);     // Load raw data from rres resource
RLAPI char *LoadTextFromResource(rresResource rres);                // Load text data from rres resource
RLAPI Image LoadImageFromResource(rresResource rres);               // Load Image data from rres resource
RLAPI Wave LoadWaveFromResource(rresResource rres);                 // Load Wave data from rres resource
RLAPI Font LoadFontFromResource(rresResource rres);                 // Load Font data from rres resource
RLAPI Mesh LoadMeshFromResource(rresResource rres);                 // Load Mesh data from rres resource
```

Note that data decompression/decryption should be implemented in this custom mapping library, `rres` only provides compressor/cipher identifier values for convenience. Compressors and ciphers support depends on user implementation and it must be aligned with the packaging tool.

`rres` file-format is engine-agnostic and a mapping library can be created for any engine/framework in any programming language.

### Packaging tool: `rrespacker`

The `rres` packing tool is in charge of processing all the input files and creating the `rres` file following the specification. In case some compression/encryption algorythm is supported it must be implemented by this tool and same algorithm should be supported by the mapping library, in our case `rres-raylib.h`.

![rres v1.0](https://raw.githubusercontent.com/raysan5/rres/master/design/rrespacker_screenshot.png)

_Fig 03. rrespacker tool, GUI interface, it also supports CLI for batch processing._

## License

`rres` file-format specs, `rres.h` library and `rres-raylib.h` library are licensed under MIT license. Check [LICENSE](LICENSE) for further details.

*Copyright (c) 2014-2022 Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
