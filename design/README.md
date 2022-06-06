# rres design concerns

This file list some of the main concerns overcomed when designing rres file format and the decisions taken on the process.

The type of concerns are organized into 4 categories:

 - 1. File Structure Concerns
 - 2. Data Types Concerns
 - 3. Implementation Concerns
 - 4. Future Concerns / Improvements

## 1. File Structure Concerns

### Input file processing or just packaging

`CONCERN`: Most packaging file-formats seem to package just files, not data; data is actually preprocessed into a custom file before packaging, should we use that approach?

`DECISION`: Resource chunks data is processed directly from input files, no intermediate formats are generated for now.

`WARNING`: Maybe this approaximation changes in the future to accomodate complex structures of data consisting of multiple chunks, instead of embedding directly all the required resource chunks one after another.

### Multi-chunk resources

`CONCERN`: Some input files could generate multiple resource chunks **when processed** (Fonts, Meshes, Models...), supporting that kind of inputs could be a complex problem.

`DECISION`: Multiple options have been analyzed and multiple redesigns of the `rres` file format have been made trying to accomodate this option. Current implementation just embeds those multiple resource chunks one after another (not mandatory) and link them through an offset resource property (`nextChunkOffset`) that links one resource to the next one.

`IMPLICATIONS`:
 - An additional field is defined on resource info header to accomodate this offset, in case offset is 0, no next resource is provided.
 - Resource reading API must consider multi-chunk resources, adding a level of complexity to implementation
 - All resource chunks share the same id, that is generated from input file.
 - Complexity!!!
 
`WARNING`: Is this the best approach? This approach could be changed in a future!
    
### Resource identifiers

`CONCERN`: We need some mechanism to identify resources once processed from input file. Beside the identifier, some mechanism to identify the type of data contained is also required.

`DECISION`: Every resource has a id generated from the input file (using input filename CRC32). Resources containing multiple chunks are identified with the same id.
Every resource also uses a `FourCC` code to identify the type of data contained, actually the `FourCC` is the first property at resource start. A `FourCC` has been chosen to have an human-readable identifier in the binary file, to identify the resources with an hex viewer if it was required or to be used as a mechanism to detect corrupted data. 
   
### Resource chunks padding

`CONCERN`: Should we consider resource chunks alignment inside file? For example, align resources to 4-byte or 8-byte memory boundaries, adding a 0-padding when required.

`DECISION`: Not required, data is already read correctly from file in memory.

`WARNING`: Maybe this decision should be reconsidered in a future.

### Filename memory alignment in `CDIR`

`CONCERN`: Should filename entries for CDIR be aligned to 4-byte or 8-byte memory bounds?

`DECISION`: Filenames in `CDIR` chunk entries are terminated with `\0` and padded with `\0` to 4-byte alignment

### Compression/Encryption support

`CONCERN`: Should data be compressed and/or encrypted?

`DECISION`: Support for data compression and/or is useful to reduce data size and protect data when required. Two fields are added to resource chunk info structure to support custom compression and encryption options BUT it's up to the rres user implementation to implement required algorythms, including the rres packer tool and the rres reading library on engine side.

`IMPLICATIONS`:
 - Should data be compressed or encrypted first?
   It is better to compress before encrypting because any proven block cipher will reduce the data to a pseudo-random sequence of bytes that will typically yield little to no compression gain at all.
   
## 2. Data Types Concerns

### Raw data resources

`CONCERN`: Input files are processed to generate the required resource chunks but there could be some cases when we want the input file unprocessed to be embedded.

`DECISION`: Define `RAWD` data type (`RRES_DATA_RAW`) for those situations. If an input file is defined to be embedded unprocessed, a `RAWD` type resource chunk is created containing the input file in raw data form. 

`IMPLICATIONS`:
 - Support a `CDIR` resource type for `Central Directory` where the original input file name is referenced, in case the original input file needs to be extracted.
 - Central Directory is not mandatory on rres file, so, some mechanism is required to identify the type of raw data contained in the `RAWD` resource for that situation:
   DECISION: Use the `RAWD` properties to codify the input file extension to identify it, every `unsigned int` property will map 4 `char` values. 

### Externally linked files

`CONCERN`: Sometimes we could be interested in linking an input file externally instead of processing or embedding it directly into the `rres` file but still, we want to keep a reference to that external file, one use case could be audio files. resources (or some chunks) could be external to the `rres` file, it would be nice to support that kind of resources.

`DECISION`: Define `LINK` data type (`RRES_DATA_LINK`) for those situations. The data will contain the input file path (mapped if applies), it should be a filepath relative to the generated `rres` file path.

`IMPLICATIONS`:
 - Should we also support externally linked resource chunks?
     Yes, it can support external link to another `rres` file containing some resource chunk

`WARNINGS`: External file could exist not exist when trying to load it.

### External resource chunk data

`CONCERN`: In the same line as referencing external files, we could be interested in allowing an external resource chunk data file (`.rresd`) containing only the processed data for the input file (and also the data properties).

`DECISION`: Not for now, let's keep it simple and self-contained.

### External central directory resource

`CONCERN`: We could be interested in keeping the Central Directory resource chunk as a separate file (`.rrcd`/`.rresi`) to be used on rres update/extraction but to avoid distributing it on final release.

`DECISION`: Not for now, let's keep it simple. Maybe in a future version.

### End-Of-File resource chunk

`CONCERN`: Support a special chunk `EOFD` data type to be placed at the end of the `rres` file as an `EOF` chunk.

`DECISION`: Not required. `rres` already contains enough information to properly separate all the contained resource chunks and actually, `FourCC` for every chunk gives users a "visual" reference to identify the different chunks contained in the file.
    
### Code file resources

`CONCERN`: Support a specific `CODE` (`RRES_DATA_CODE`) resource chunk type to identify input code files.

`DECISION`: Not required. We will try to keep data as generic as possible, code files could be just embedded as `TEXT` (`RRES_DATA_TEXT`) resource chunks and an additional property could be added to identify the code language of the text data.

### Zero-terminated `TEXT` resources

`CONCERN`: Input files processed as `TEXT` type resources, should they be `\0` terminated when saved data?

`DECISION`: At this moment they are not, it's up to the user to allocate an extra byte (`\0`) to zero-terminate the loaded data chunk.

## 3. Implementation Concerns

### Make rres file-format engine-agnostic

`CONCERN`: `rres` file format is designed to work with any engine/framework but an actual implementation is required to read/write resource chunks data and map that data properly to the data structures provided by the different engines. Sooner or later a custom implementation is required.

`DECISION`: Implement a base `rres.h` library (single-file, header-only, no-dependencies) to read resources chunk data as _raw_ data and also create an engine/framework specific library to map the read _raw_ and properties to the engine structures. (i.e. `rres-raylib.h`)

### Compression/Encryption support

`CONCERN`: `rres` is designed to support compression/encryption of data when required but compression/encryption algorithms could be highly dependant on user needs and the target engine/framework used.

`DECISION`: Do not force any compression/encryption algorithm, just let the rres packer tool **and** the engine/framework specific library to implement desired compression/encryption schemes.

`IMPLICATIONS`: Base library `rres.h` does not support decompression/decryption on resource chunk reading, it just returns the compressed/encrypted raw data to allow the engine/framework specific library to process using the supported algorithms.

### Expose rres structures directly to the user

`CONCERN`: Should base library `rres.h` provided data structures (`rresResourceChunkInfo` + `rresResourceChunkData`) be directly exposed to users.

`DECISION`: After a first implementation not exposing them directly (only custom trimmed versions), I decided to expose them completely in a second implementation, it really simplifies the implementation and it becomes more intuitive for users, having a direct equivalence between the `rres` format specification and the implementation.

`IMPLICATIONS`:

Previous implementation just exposed some of the `rresResourceChunkInfo` properties and converted the `unsigned char type[4]` (FourCC) to a more user-friendly `unsigned int type`, but it added a level of confusion and miss-alignment between the exposed `rresResourceChunk` and the structure defined in the specs. Now both are aligned.
 - The reasoning for previous implementation: Just expose the minimal required properties for the user and keep the non-required ones private to the library implementation.
 - The reasoning for new implementation: Compression/encryption was moved to user library, so, several of the properties were already exposed. 
   The fields `flags`, `nextOfsset` and `crc32` could also be useful for user at some point so, why not expose them? And definitely, 
   rres is an open specification and `rres.h` implementation is open source, there is no need to hide information to the users! 
   And also the alignment betweent the specs and what user gets. The only degradation is the `type`, the user now gets a `unsigned char type[4]` 
   instead of a `unsigned int type`, but `rresGetDataType()` function has been added to help (a bit) on that line.

## 4. Future Concerns / Improvements

Following concerns have arised after the `rres` first design and implementation and could be a foundation for an improvement of the format based on the experience.

### Multi-chunk resources

`CONCERN`: Some input files could generate multiple resource chunks, all linked together through a `nextChunkOffset` parameter in the resource chunk info header. All chunks generated from a unique input file share the same id. Despite the result is quite elegant in file-format terms (one chunk after another), it really complicates the implementation to read/write that kind of multi-chunk resources. Specific functions are required to return multiple chunks array while most of the time users will probably only want to load a single chunk (more simple and intuitive approach). Multiple chunks sharing same id could also sound confusing.

`DECISION`: Package multi-chunk resources into a special kind of resource and let the user load the chunks after loading the individual resource. Another approach could be similar to other packaging formats, just create an intermediate multi-chunk file and embed that file as `RAWD` type.

### Number of data properties

`CONCERN`: First `rres` design assumed a fixed number of 4 properties per resource chunk; it was changed later to support any number of properties, just storing the properties count as first parameter of data chunk. After implementing the libraries to write and read `rres` files just noted that most resource chunks do not need more than 4 properties to identify data properly, so, we end-up in most of the cases with 5 integers: propsCount + props[4]. Implementation implies dynamic memory loading of those properties that adds a level of complexity that, maybe, could be avoided,

`DECISION`: Set a default number of 8 properties for each resource chunk, that way, all resource chunks data will always start with 32 bytes of properties, fixed size. It will simplify read/write processes and dynamic memory allocations could be avoided.
