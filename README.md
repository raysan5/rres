<img align="left" src="https://github.com/raysan5/rres/blob/master/logo/rres_256x256.png" width=256>

<br>

**rres is a simple and easy-to-use file-format to package resources**

`rres` has been designed to package game assets (images, fonts, text, audio, models...) into a simple self-contained comprehensive format, easy to read and write, prepared to load data in a fast and efficient way.

`rres` is inspired by [XNB](http://xbox.create.msdn.com/en-US/sample/xnb_format) file-format (used by XNA/MonoGame), [RIFF](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format), [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) and [ZIP](https://en.wikipedia.org/wiki/Zip_(file_format)) file-formats.

<br>
<br>

## rres history

rres has been in development **since 2014**. I started this project with the aim to create a packaging file-format similar to [XNB](http://xbox.create.msdn.com/en-US/sample/xnb_format) for raylib. In the last **8 years** the project has suffered multiple redesigns and improvements along a file-formats learning process. In that time I implemented loaders/writers for **+20 different file formats** and also created **+12 custom file formats** for multiple [raylibtech custom tools](https://raylibtech.itch.io/).

rres file-format has gone through _at least_ **4 big redesigns**:

 - **First design** of the format was limited to packaging one resource after another, every resource consisted of one _resource info header_ followed by a fixed set of four possible parameters and the resource data. Along the `.rres` file, a `.h` header file was generated mapping with defines the `rresId` with a resource filename (usually the original filename of the un-processed data). This model was pretty simple and intuitive but it has some important downsides: it did not consider complex pieces of data that could require multiple chunks and there was no way to extract/retrieve original source files (or similar ones).

 - **Second design** was way more complex and tried to address first design shortcomings. In second design every resource could include multiple chunks of data in a kind of tree structure. Actually, that design was similar to [RIFF](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format) file-format: every resource in the package could be threatened as a separate file on its own. Some additional resource options were also added but the format become quite complex to understand and manage. Finally the testing implementation was discarded and a simpler alternative was investigated.

 - **Third design** was a return to first design: simplicity but keeping some of the options for the individual resources. The problem of multiple resources generated from a single input file was solved using a simple offset field in the _resource info header_ pointing to next linked resource when required. Resources were loaded as an array of resource chunks. An optional Central Directory resource chunk was added to keep references for the input files. Format was good but it still required implementation and further investigation, it needed to be engine-agnostic and it was still dependant on raylib structures and functionality.

 - **Fourth design** has been done along the implementation, almost all structures and fields have been reviewed and renamed for consistency and simplicity. A separare library has been created (`rres-raylib.h`) to map the loaded resources data into a custom library/engine data types, any depndency to raylib has been removed, making it a completely engine-agnostic file-format. Several usage examples for raylib has been implemented to illustrate the library-engine connection of `rres` and multiple types of resource loading have been implemented. `rrespacker` tool has been created from scratch to generate `rres` files, it supports a nive GUI interface with drag and drop support but also a powerful command-line for batch processing. Compression and encryption implementation has been moved to the user library implementation to be aligned with the packaging tool and keep the `rres` file-format cleaner and simpler.

It's been an **8 years project**, working on it on-and-off, with many redesigns and revisions but I'm personally very happy with the final result. `rres` is a resource packaging file-format at the level of any professional engine package format _BUT free and open-source_, available to any game developer that wants to use it, implement it or create a custom version.

## rres design goals

- **Simplicity**: rres structure is simple, one file header and multiple resources one after the other. Still, every resource has a small (32 bytes) resource info header with multiple options available.

- **Ease-of-use**: rres library to read rres files is small with only the minimum required functions to read resource data from it's id. It only uses a small set of functions from the standard C library. An `rrespacker` tool with GUI/CLI is provided to easily create and view .rres files.

- **Flexibility**: rres format support any kind of input file, if data is not classified as a basic data type it can be just packed as raw file data.

- **Portability**: rres file format is not tied to an specific engine, the base library just reads the resource data as packed and that data can be mapped to any engine structures. An usage example has been provided with the auxiliar library `rres-raylib.h` that maps rres data to raylib structures.

- **Security**: rres support per-resource compression and encryption if required. Still, despite the support provided by the file format is up to the user to implement it in the rres packer tool and the engine mapping library. rres does not force any specific compression/encryption algorithm by design

- **Extensibility**: rres file format is scalable, supporting new data types, new features and custom data if required.

- **Free and open source**: rres is distributed as an open spec and a free and open source library. It can be used with the provided tool (`rrespacker`) or custom packer/loader implementations could be created for **any engine**. It can also be used as a learning material for anyone willing to create its own data packaging file format.

## rres usage benefits

There are some important reasons to package game assets data into a format like `rres`, here some good reasons to do it.

 - **Organization**: Avoid thousands of files distributed in hundreds of directories in the final game build. All the game files data could be organized in a single or a few `.rres` files. It keeps the game directory clean and well structured.

 - **Performance**: Keeping all required files for a level/map into a single `.rres` file reduces the need of file handles to access multiple files; file handles are an OS resource and the cost of opening a file is not insignificant, it may cause delays in the game. Data access times inside the `.rres` file are also important and depending how the files are organized it could also improve the loading times.

 - **Security**: Avoid exposing all the game assets directly to the user for easy copy or modification. Data packed into a `.rres` file will be more difficult to extract and modify for most users; protect your game copyrighted assets. `rres` also supports per-resource data compression and encryption, adding an extra level of security when required. 

 - **Downloading times**: If data needs to be accessed from a server and downloaded, the assets packed into a single or a few `.rres` files improves download times, in comparison to downloading each file individually.

## rres file structure

rres file format consists of a file header (`rresFileHeader`) followed by a number of resource chunks (`rresResourceChunk`). Every resource chunk has a resource info header (`rresResourceInfoHeader`) that includes a `FOURCC` data type code and resource data information. The resource data (`rresResourceDataChunk`) contains a small set of properties to identify data, depending on the type and could contain some additional data at the end.

![rres v1.0](https://raw.githubusercontent.com/raysan5/rres/master/design/rres_file_format_REV5.png)

_rres v1.0 file structure._

_NOTE: Resource chunks are usually generated from input files. It's important to note that resources could not be mapped to files 1:1, one input file could generate multiple resource chunks. For example, a .ttf input file could generate an image resource chunk (`RRES_DATA_IMAGE`) plus a font glyph info resource chunk (`RRES_DATA_GLYPH_INFO`)._


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
        Id                    (4 bytes)     // Resource identifier (CRC32 filename hash)
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

## rres file header: `rresFileHeader`

The following C struct defines the `rresFileHeader`:

```c
// rres file header (16 bytes)
typedef struct rresFileHeader {
    unsigned char id[4];            // File identifier: rres
    unsigned short version;         // File version: 100 for version 1.0.0
    unsigned short chunkCount;      // Number of resource chunks in the file (MAX: 65535)
    unsigned int cdOffset;          // Central Directory offset in file (0 if not available)
    unsigned int reserved;          // <reserved>
} rresFileHeader;
```

Considerations:

 - rres files are limited by design to a maximum of 65535 resource chunks, in case more resources need to be packed is recommended to create multiple rres files.
 - rres files use 32 bit offsets to address the different resource chunks, consequently, no more than ~4GB of data can be addressed, please keep the rres files smaller than 4GB. In case of more space required to package resources, create multiple rres files.
 - Central directory is just another resource chunk, it can be present in the file or not. It's recommended to be the ast chunk in the file. More info on `rresCentralDir` section.

## rres resource chunk: `rresResourceChunk`

rres file contains a number of resource chunks. Every resource chunk represents a self-contained pack of data. Resource chunks are generated from input files on rres file creation by the rres packer tool; depending on the file extension, the rres packer tool extracts the required data from the file and generates one or more resource chunks. For example, for an image file, a resource chunk of `type` `RRES_DATA_IMAGE` is generated containing only the pixel data of the image and the required properties to read that data back from the resource file.

It's important to note that one input file could generate several resource chunks when the rres file is created. For example, a .ttf input file could generate an `RRES_DATA_IMAGE` resource chunk plus a `RRES_DATA_GLYPH_INFO` resource chunk; it's also possible to just pack the file as a plain `RRES_DATA_RAW` resource chunk type, in that case the input file is not processed, just packed as raw data.

On rres creation, rres packer could create an additional resource chunk of type `RRES_DATA_DIRECTORY` containing data about the processed input files. It could be useful in some cases, for example to relate the input filename directly to the generated resource(s) id and also to extract the data in a similar file structure to the original input one.

Every resource chunk is divided in two part: `rresResourceInfoHeader` + `rresResourceData`.

### rres resource chunk info header: `rresResourceInfoHeader`

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

Considerations:

 - `type` is a [`FourCC`](https://en.wikipedia.org/wiki/FourCC) code and identifies the type of data contained in the resource chunk. Check `rresResourceDataType` section.
 - `id` is a global resource identifier, it's generated from input filename using a CRC32 hash and it's not unique. One input file can generate multiple resource chunks, all the generated chunks share the same identifier and they are loaded together when the resource is loaded. For example, a input .ttf could generate two resource chunks (`RRES_DATA_IMAGE` + `RRES_DATA_GLYPH_INFO`) with same identifier that will be loaded together when their identifier is requested. It's up to the user to decide what to do with loaded data.
 - `compType` defines the compression algorithm used for the resource chunk data. Compression depends on the middle library between rres and the engine, `rres.h` just defines some useful algorithm values to be used in case of implementing compression. Compression should always be applied before encryption and it compresses the full `rresResourceData` (`Property Count` + `Properties[]` + `Data`). If no data encryption is applied, `packedSize` defines the size of compressed data.
 - `cipherType` defines the encryption algorithm used for the resource chunk data. Like compression, encryption depends on the middle library between rres and the engine, `rres.h` just defines some useful algorithm values to be used in case of implementing encryption. Encryption should be applied after compression. Depending on the encryption algorithm and encryption mode it could require some extra piece of data to be attached to the resource data (i.e encryption MAC), this is implementation dependant and the rres packer tool / rres middle library for the engines are the responsible to manage that extra data. It's recommended to be just appended to resource data and considered on `packedSize`.
 - `flags` is reserved for additional flags, in case they are required by the implementation.
 - `nextOffset` defines the global file position address for the next _related_ resource chunk, it's useful for input files that generate multiple resources, like fonts or meshes.
 - `crc32` is useful for a quick check of data corruption, by default it considers the `rresResourceData` chunk.

### rres resource data chunk: `rresResourceDataChunk`

`rresResourceDataChunk` contains the following data:

 - `Property Count`: Number of properties contained, depends on resource `type`
 - `Properties[]`: Resource data required properties, depend on resource `type`
 - `Data`: Resource data, depend on resource `type`
 
Considerations:
 
 - `rresResourceInfoHeader.type` defines the type of resource, one of the types contained in enum `rresResourceDataType`.
 - `rresResourceInfoHeader.baseSize` defines the base size (uncompressed/unencrypted) of `rresResourceDataChunk`. 
 - `rresResourceInfoHeader.packedSize` defines the compressed/encrypted size of `rresResourceDataChunk`. It could also consider any extra data appended to `rresResourceData` (i.e. encryption MAC) but it is implementation dependant.
 
### rres resource data types: `rresResourceDataType`

The resource `type` specified in the `rresResourceInfoHeader` defines the type of data and the number of properties contained in the resource chunk.

Here it is the currently defined data types. Please note that some input files could generate multiple resource chunks of multiple types. Additional resource types could be added if required.

```c
// rres resource chunk data type
// NOTE 1: Data type determines the properties and the data included in every chunk
// NOTE 2: This enum defines the basic resource data types, some input files could generate multiple resource chunks
typedef enum rresResourceDataType {
    // Basic data (one chunk)
    //-----------------------------------------------------
    RRES_DATA_NULL         = 0,     // FourCC: NULL - Reserved for empty chunks, no props/data
    RRES_DATA_RAW          = 1,     // FourCC: RAWD - Raw file data
    RRES_DATA_TEXT         = 2,     // FourCC: TEXT - Text file data
    RRES_DATA_IMAGE        = 3,     // FourCC: IMGE - Image file data
    RRES_DATA_WAVE         = 4,     // FourCC: WAVE - Audio file data
    RRES_DATA_VERTEX       = 5,     // FourCC: VRTX - Mesh file data
    RRES_DATA_GLYPH_INFO   = 6,     // FourCC: FNTG - Font file processed, glyphs data
    RRES_DATA_LINK         = 99,    // FourCC: LINK - External linked file
    RRES_DATA_DIRECTORY    = 100,   // FourCC: CDIR - Central directory for input files

} rresResourceDataType;
```

The current data types and its properties are:

 - `RRES_DATA_NULL`:`NULL`: This is an empty data chunk (no `rresResourceData`), reserved for special use-cases
   - Properties Count: 0
   - Properties: none
   - Data: none
 - `RRES_DATA_RAW`:`RAWD`: Raw data, the input file not processed and packed as is.
   - Properties Count: 1
   - Properties: `props[0]`:size
   - Data: raw_data
 - `RRES_DATA_TEXT`:`TEXT`: Text data, byte data extracted from text file
   - Properties Count: 4
   - Properties: `props[0]`:size, `props[1]`:rresTextEncoding, `props[2]`:rresCodeLang, `props[1]`:cultureCode
   - Data: text_data
 - `RRES_DATA_IMAGE`:`IMGE`: Image data, pixel data extracted from image file
   - Properties Count: 4
   - Properties: `props[0]`:width, `props[1]`:height, `props[2]`:`rresPixelFormat`, `props[3]`:mipmaps 
   - Data: pixel_data
 - `RRES_DATA_WAVE`:`WAVE`: Wave audio data, samples data extracted from audio file
   - Properties Count: 4
   - Properties: `props[0]`:sampleCount, `props[1]`:sampleRate, `props[2]`:sampleSize, `props[3]`:channels
   - Data: audio_samples_data
 - `RRES_DATA_VERTEX`:`VRTX`: Vertex data, extracted from a mesh file
   - Properties Count: 4
   - Properties: `props[0]`:vertexCount, `props[1]`:`rresVertexAttribute`, `props[2]`:componentCount, `props[2]`:`rresVertexDataFormat`
   - Data: vertex_data
 - `RRES_DATA_GLYPH_INFO`:`FNTG`: Font glyphs info, generated from an input font file
   - Properties Count: 4
   - Properties: `props[0]`:baseSize, `props[1]`:glyphCount, `props[2]`:glyphPadding, `props[3]`:rresFontStyle
   - Data: glyph_data: `rresFontGlyphInfo[0..glyphCount]`
 - `RRES_DATA_LINK`:`LINK`: Filepath link to an external file, as provided on file input
   - Properties Count: 1
   - Properties: `props[0]`:size
   - Data: link_filepath_data
 - `RRES_DATA_DIRECTORY`:`CDIR`: Central directory for resource chunks, keep information about the input files and relation to resources.
   - Properties Count: 1
   - Properties: `props[0]`:entryCount
   - Data: directory_data: `rresDirEntry[0..entryCount]`

### rres central directory resource chunk

The Central Directory resource chunk is a special chunk that could be present or not in the rres file, it stores information about the input files processed to generate the multiple resource chunks and could be useful for:
 - Extract some of the resources to a similar input file. It will be only possible if the input file has not been processed in a destructive way. For example, if  a.ttf file has been processed to generate a glyphs image atlas + glyphs data info, it won't be possible to retrieve the original .ttf file.
 - Reference the resources by its original filename. This is very useful in terms of implementation to minimize the required code changes if rres packaging is done over an existing project or at the end of a project where all files loading has been done using directly filenames. 
 
rres provides some helpful structures to deal with Central Directory resource chunk data:

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
 
`rres.h` provides a function to load Central Directory from rres file when available: `rresLoadCentralDirectory()` and also a function to get a resource identifiers from its original filename: `rresGetIdFromFileName()`.

In case a `rres` file is generated with no Central Directory, a secondary header file (`.h`) should be provided with the id references for all resources, to be used in user code.

## rres implementation for a custom engine

`rres` is designed as a engine-agnostic file format, data is mostly threated as generic data, common to any game engine. `rres` users can implement simple abstraction layers to map `rres` generic data to their own engines data structures. 

`rres-raylib.h` is an example of a `rres` implementation for [`raylib`](https://github.com/raysan5/raylib). It maps the different `rres` data types to `raylib` data structures. The API provided is simple and intuitive:

```c
RRESAPI void *rresLoadRaw(rresResource rres, int *size);    // Load raw data from rres resource
RRESAPI char *rresLoadText(rresResource rres);              // Load text data from rres resource
RRESAPI Image rresLoadImage(rresResource rres);             // Load Image data from rres resource
RRESAPI Wave rresLoadWave(rresResource rres);               // Load Wave data from rres resource
RRESAPI Font rresLoadFont(rresResource rres);               // Load Font data from rres resource
RRESAPI Mesh rresLoadMesh(rresResource rres);               // Load Mesh data from rres resource
```

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
```

A similar mapping library can be created for any other engine/framework.

## rres specs and library license

`rres` file-format specs and `rres.h` library are licensed under MIT license. Check [LICENSE](LICENSE) for further details.

*Copyright (c) 2014-2022 Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
