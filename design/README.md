# rres design concerns

This file list some of the main concerns overcomed when designing rres file format and the decisions took on the process.

## File Structure Concerns

### Input file processing or just packaging

CONCERN:
Most packaging file-formats seem to package just files, not data; data is actually preprocessed into a custom file before packaging, should we use that approach?

DECISION:
`rres` resource chunks data is processed directly from input files, no intermediate formats are generated for now.

WARNING:
Maybe this approaximation changes in the future to accomodate complex structures of data consisting of multiple chunks, instead of embedding directly all the required resource chunks one after another.

### Multi-chunk resources

CONCERN:
Some input files could generate multiple resource chunks **when processed** (Fonts, Meshes, Models...), supporting that kind of inputs could be a complex problem.

DECISION:
Multiple options have been analyzed and multiple redesigns of the `rres` file format have been made trying to accomodate this option. Current implementation just embeds those multiple resource chunks one after another (not mandatory) and link them through an offset resource property (`nextChunkOffset`) that links one resource to the next one.

IMPLICATIONS:
 - An additional field is defined on resource info header to accomodate this offset, in case offset is 0, no next resource is provided.
 - Resource reading API must consider multi-chunk resources, adding a level of complexity to implementation
 - All resource chunks share the same id, that is generated from input file.
 - Complexity!!!
 
WARNING:
Is this the best approach? This approach could be changed in a future!
    
### Resource identifiers

CONCERN:
We need some mechanism to identify resources once processed from input file. Beside the identifier, some mechanism to identify the type of data contained is also required.

DECISION:
Every resource has a id generated from the input file (using input filename CRC32). Resources containing multiple chunks are identified with the same id.
Every resource also uses a `FourCC` code to identify the type of data contained, actually the `FourCC` is the first property at resource start. A `FourCC` has been chosen to have an human-readable identifier in the binary file, to identify the resources with an hex viewer if it was required or to be used as a mechanism to detect corrupted data. 
   
### Resource chunks padding

CONCERN:
Should we consider resource chunks alignment inside file? For example, align resources to 4-byte or 8-byte memory boundaries, adding a 0-padding when required.

DECISION:
Not required, data is already read correctly from file in memory.

WARNING:
Maybe this decision should be reconsidered in a future.

### Filename memory alignment in CDIR

CONCERN:

DECISION:
Filenames in CDIR are padded with '\0' to 4-byte alignment of entries

### Compression/Encryption support

CONCERN:
Should data be compressed and/or encrypted?

DECISION:
Support for data compression and/or is useful to reduce data size and protect data when required. Two fields are added to resource chunk info structure to support custom compression and encryption options BUT it's up to the rres user implementation to implement required algorythms, including the rres packer tool and the rres reading library on engine side.

IMPLICATIONS:
 - Should data be compressed or encrypted first?
   It is better to compress before encrypting because any proven block cipher will reduce the data to a pseudo-random sequence of bytes that will typically yield little to no compression gain at all.
   
## Data Types Concerns

### Raw data resources

CONCERN:
Input files are processed to generate the required resource chunks but there could be some cases when we want the input file unprocessed to be embedded.

DECISION: 
Define `RAWD` data type (`RRES_DATA_RAW`) for those situations. If an input file is defined to be embedded unprocessed, a `RAWD` type resource chunk is created containing the input file in raw data form. 

IMPLICATIONS:
 - Support a `CDIR` resource type for `Central Directory` where the original input file name is referenced, in case the original input file needs to be extracted.
 - Central Directory is not mandatory on rres file, so, some mechanism is required to identify the type of raw data contained in the `RAWD` resource for that situation:
   DECISION: Use the `RAWD` properties to codify the input file extension to identify it, every `unsigned int` property will map 4 `char` values. 

### Externally linked files

CONCERN:
Sometimes we could be interested in linking an input file externally instead of processing or embedding it directly into the `rres` file but still, we want to keep a reference to that external file, one use case could be audio files. resources (or some chunks) could be external to the `rres` file, it would be nice to support that kind of resources.

DECISION:
Define `LINK` data type (`RRES_DATA_LINK`) for those situations. The data will contain the input file path (mapped if applies), it should be a filepath relative to the generated `rres` file path.

IMPLICATIONS:
 - Should we also support externally linked resource chunks?
     Yes, it can support external link to another `rres` file containing some resource chunk

WARNINGS: 
External file could exist not exist when trying to load it.

### External resource chunk data

CONCERN:
In the same line as referencing external files, we could be interested in allowing an external resource chunk data file (`.rresd`) containing only the processed data for the input file (and also the data properties).

DECISION:
Not for now, let's keep it simple and self-contained.

### External central directory resource

CONCERN:
We could be interested in keeping the Central Directory resource chunk as a separate file (`.rrcd`/`.rresi`) to be used on rres update/extraction but to avoid distributing it on final release.

DECISION:
Not for now, let's keep it simple. Maybe in a future version.

### End-Of-File resource chunk

CONCERN:
Support a special chunk `EOFD` data type to be placed at the end of the `rres` file as an `EOF` chunk.

DECISION: 
Not required. `rres` already contains enough information to properly separate all the contained resource chunks and actually, `FourCC` for every chunk gives users a "visual" reference to identify the different chunks contained in the file.
    
### Code file resources

CONCERN:
Support a specific `CODE` (`RRES_DATA_CODE`) resource chunk type to identify input code files.

DECISION:
Not required. We will try to keep data as generic as possible, code files could be just embedded as `TEXT` (`RRES_DATA_TEXT`) resource chunks and an additional property could be added to identify the code language of the text data.
