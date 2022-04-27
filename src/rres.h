/**********************************************************************************************
*
*   rres v1.0 - A simple and easy-to-use file-format to package resources
*
*   CONFIGURATION:
*
*   #define RRES_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*
*   FILE STRUCTURE:
*
*   rres files consist of a file header followed by a number of resource chunks.
* 
*   Optionally it can contain a Central Directory resource chunk at the end with the info 
*   of all the files processed into the rres file.
* 
*   NOTE: Chunks count could not match files count, some processed files (i.e Font, Mesh) 
*   generate multiple chunks with the same id related by the rresResourceInfoHeader.nextOffset and 
*   loaded together when resource is loaded
*
*   rresFileHeader               (16 bytes)
*       Signature Id              (4 bytes)     // File signature id: 'rres'
*       Version                   (2 bytes)     // Format version
*       Resource Count            (2 bytes)     // Number of resource chunks contained
*       CD Offset                 (4 bytes)     // Central Directory offset (if available)
*       reserved                  (4 bytes)     // <reserved>
*
*   rresResourceChunk[]
*   {
*       rresResourceInfoHeader   (32 bytes)
*           Type                  (4 bytes)     // Resource type (FourCC)
*           Id                    (4 bytes)     // Resource identifier (CRC32 filename hash)
*           Compressor            (1 byte)      // Data compression algorithm
*           Cipher                (1 byte)      // Data encryption algorithm
*           Flags                 (2 bytes)     // Data flags (if required)
*           Packed data Size      (4 bytes)     // Packed data size (compressed/encrypted)
*           Base data Size        (4 bytes)     // Base data size (uncompressed/decrypted)
*           Next Offset           (4 bytes)     // Next resource chunk offset (if required)
*           Reserved              (4 bytes)     // <reserved>
*           CRC32                 (4 bytes)     // Resource Data Chunk CRC32
*                                
*       rresResourceDataChunk     (n bytes)     // Packed data
*           Property Count        (4 bytes)     // Number of properties contained
*           Properties[]          (4*i bytes)   // Resource data required properties, depend on Type
*           Data                  (m bytes)     // Resource data
*           Extra Data            (p bytes)     // OPTIONAL: Additional data when required (i.e. encryption MAC)
*   }
*
*   rresResourceChunk: RRES_DATA_DIRECTORY      // Central directory (special resource chunk)
*   {
*       rresResourceInfoHeader   (32 bytes)
*
*       rresCentralDir            (n bytes)     // rresResourceDataChunk
*           Entries Count         (4 bytes)     // Central directory entries count (files)
*           rresDirEntry[]
*           {
*               Id                (4 bytes)     // Resource id
*               Offset            (4 bytes)     // Resource global offset in file
*               reserved          (4 bytes)     // <reserved>
*               FileName Size     (4 bytes)     // Resource fileName size (NULL terminator and 4-bytes align padding considered)
*               FileName          (m bytes)     // Resource original fileName (NULL terminated and padded to 4-byte alignment)
*           }
*    }
*
*   DEPENDENCIES:
*
*   This file dependencies has been keep to the minimum. It depends only some libc functionality:
*
*     - stdlib.h: Required for memory allocation: malloc(), calloc(), free()
*                 NOTE: Allocators can be redefined with macros RRES_MALLOC, RRES_CALLOC, RRES_FREE
*     - stdio.h:  Required for file access functionality: FILE, fopen(), fseek(), fread(), fclose()
*     - string.h: Required for memory data mamagement: memcpy(), strcmp() 
*
*   VERSIONS HISTORY:
*
*     - 1.0 (28-Apr-2022): Official release of rres 1.0
*
*
*   DESIGN DECISIONS / LIMITATIONS:
*
*     - rres file maximum chunks: 65535 (16bit chunk count in rresFileHeader)
*     - rres file maximum size: 4GB (chunk offset and Central Directory Offset is 32bit, so it can not address more than 4GB
*     - Chunk search by ID is done one by one, starting at first chunk and accessed with fread() function
*     - Endianness: rres does not care about endianness, data is stored as desired by the host platform (most probably Little Endian)
*       Endianness won't affect chunk data but it will affect rresFileHeader and rresResourceInfoHeader
*     - CRC32 hash is used to to generate the rres file identifier from filename
*       There is a "small" probability of random collision (1 in 2^32 approx.) but considering 
*       the chance of collision is related to the number of data inputs, not the size of the inputs, we assume that probablility
*       Also note that CRC32 is not used as a security/cryptographic hash, just an identifier for the input file
*     - CRC32 hash is also used to detect chunk data corruption. CRC32 is smaller and computationally much less complex than MD5 or SHA1.
*       Using a hash function like MD5 is probably overkill for random error detection
*     - Central Directory rresDirEntry.fileName is NULL terminated and padded to 4-byte, rresDirEntry.fileNameSize considers full byte size
*     - Compression and encryption. rres supports chunks data compression and encryption, it provides to fields in the rresResourceInfoHeader to
*       note it, but in those cases is up to the user to implement the desired compressor/uncompressor and encryption/decription mechanisms
*       In case of data encryption, it's recommended that any additional resource data (i.e. MAC) to be appended to data chunk and properly
*       noted in the data size in the rresResourceInfoHeader.
*       Data compression should be applied before encryption
*
*
*   LICENSE: MIT
*
*   Copyright (c) 2016-2022 Ramon Santamaria (@raysan5)
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*   
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*   
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
**********************************************************************************************/

#ifndef RRES_H
#define RRES_H

// Function specifiers in case library is build/used as a shared library (Windows)
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
#if defined(_WIN32)
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RRESAPI __declspec(dllexport)     // We are building the library as a Win32 shared library (.dll)
    #elif defined(USE_LIBTYPE_SHARED)
        #define RRESAPI __declspec(dllimport)     // We are using the library as a Win32 shared library (.dll)
    #endif
#endif

// Function specifiers definition
#ifndef RRESAPI
    #define RRESAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

// Allow custom memory allocators
#ifndef RRES_MALLOC
    #define RRES_MALLOC(sz)         malloc(sz)
#endif
#ifndef RRES_CALLOC
    #define RRES_CALLOC(ptr,sz)     calloc(ptr,sz)
#endif
#ifndef RRES_REALLOC
    #define RRES_REALLOC(ptr,sz)    realloc(ptr,sz)
#endif
#ifndef RRES_FREE
    #define RRES_FREE(ptr)          free(ptr)
#endif

// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define RRES_SUPPORT_LOG_INFO
#if defined(RRES_SUPPORT_LOG_INFO)
    #define RRES_LOG(...) printf(__VA_ARGS__)
#else
    #define RRES_LOG(...)
#endif

#define RRES_MAX_CDIR_FILENAME_LENGTH  512

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rres resource chunk
// WARNING: This is the resource type returned to the user after
// reading and processing data from file, it's not directly aligned with
// internal resource data types: rresResourceInfoHeader + rresResourceDataChunk
// It just returns the data required to process and load the resource by the engine library
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
// NOTE: It could consist of multiple chunks
typedef struct rresResource {
    unsigned int count;             // Resource chunks count
    rresResourceChunk *chunks;      // Resource chunks
} rresResource;

// Specific data for some chunk types
//----------------------------------------------------------------------
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

// rres font glyphs info (32 bytes)
typedef struct rresFontGlyphInfo {
    int x, y, width, height;        // Glyph rectangle in the atlas image
    int value;                      // Glyph codepoint value
    int offsetX, offsetY;           // Glyph drawing offset (from base line)
    int advanceX;                   // Glyph advance X for next character
} rresFontGlyphInfo;

// rres resource chunk data type
// NOTE: Data type determines the proporties and the data included in every chunk
typedef enum rresResourceDataType {
    // Basic data (one chunk)
    //-----------------------------------------------------
    RRES_DATA_NULL         = 0,     // [NULL] Reserved for empty chunks (if required)
    RRES_DATA_RAW          = 1,     // [RAWD] props[0]:size | data: raw bytes
    RRES_DATA_TEXT         = 2,     // [TEXT] props[0]:size, props[1]:cultureCode | data: text
    RRES_DATA_IMAGE        = 3,     // [IMGE] props[0]:width, props[1]:height, props[2]:mipmaps, props[3]:rresPixelFormat | data: pixels
    RRES_DATA_WAVE         = 4,     // [WAVE] props[0]:sampleCount, props[1]:sampleRate, props[2]:sampleSize, props[3]:channels | data: samples
    RRES_DATA_VERTEX       = 5,     // [VRTX] props[0]:vertexCount, props[1]:rresVertexAttribute, props[2]:rresVertexFormat | data: vertex
    RRES_DATA_GLYPH_INFO   = 6,     // [FNTG] props[0]:baseSize, props[1]:glyphsCount, props[2]:glyphsPadding | data: rresFontGlyphInfo[0..glyphsCount]
    RRES_DATA_LINK         = 99,    // [LINK] props[0]:size | data: path (relative to .rres file)
    RRES_DATA_DIRECTORY    = 100,   // [CDIR] props[0]:entryCount | data: rresDirEntry[0..entryCount]

    // Complex data (multiple chunks)
    //-----------------------------------------------------
    // Font consists of (2) chunks:
    //  - [FNTG] rres[0]: RRES_DATA_GLYPH_INFO
    //  - [IMGE] rres[1]: RRES_DATA_IMAGE
    //
    // Mesh consists of (n) chunks:
    //  - [VRTX] rres[0]: RRES_DATA_VERTEX
    //  ...
    //  - [VRTX] rres[n]: RRES_DATA_VERTEX

    // NOTE: New rres data types can be added here with custom props|data

} rresResourceDataType;

// TODO: rres error codes
// NOTE: Error codes when processing rres files
typedef enum rresErrorType {
    RRES_SUCCESS = 0,               // rres file loaded/saved successfully
    RRES_ERROR_FILE_NOT_FOUND,      // rres file can not be opened (spelling issues, file actually does not exist...)
    RRES_ERROR_FILE_FORMAT,         // rres file format not a supported (wrong header, wrong identifier)
    RRES_ERROR_MEMORY_ALLOC,        // Memory could not be allocated for operation.
} rresErrorType;

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

// Load all data chunks for specified rresId
RRESAPI rresResource rresLoadResource(const char *fileName, int rresId);
RRESAPI void rresUnloadResource(rresResource rres);

RRESAPI rresCentralDir rresLoadCentralDirectory(const char *fileName);
RRESAPI void rresUnloadCentralDirectory(rresCentralDir dir);

RRESAPI int rresGetIdFromFileName(rresCentralDir dir, const char *fileName);
RRESAPI unsigned int rresComputeCRC32(unsigned char *buffer, int len);

#ifdef __cplusplus
}
#endif

#endif // RRES_H


/***********************************************************************************
*
*   RRES IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES_IMPLEMENTATION)

#include <stdlib.h>                 // Required for: malloc(), free()
#include <stdio.h>                  // Required for: FILE, fopen(), fseek(), fread(), fclose()
#include <string.h>                 // Required for: memcpy(), strcmp()

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rres file header (16 bytes)
typedef struct rresFileHeader {
    unsigned char id[4];            // File identifier: rres
    unsigned short version;         // File version: 100 for version 1.0.0
    unsigned short chunkCount;      // Number of resource chunks in the file (MAX: 65535)
    unsigned int cdOffset;          // Central Directory offset in file (0 if not available)
    unsigned int reserved;          // <reserved>
} rresFileHeader;

// rres resource chunk info header (32 bytes)
typedef struct rresResourceInfoHeader {
    unsigned char type[4];          // Resource chunk type (FourCC)
    unsigned int id;                // Resource chunk identifier (generated from filename CRC32 hash)
    unsigned char compType;         // Data compression algorithm
    unsigned char cipherType;       // Data encription algorithm
    unsigned short flags;           // Data flags (if required)
    unsigned int packedSize;        // Data chunk size (data can be compressed or include additional data appended)
    unsigned int baseSize;          // Data base size (uncompressed and not considering additional data appended)
    unsigned int nextOffset;        // Next resource chunk global offset (if resource has multiple chunks)
    unsigned int reserved;          // reserved
    unsigned int crc32;             // Data chunk CRC32 (propCount + props[] + data)
} rresResourceInfoHeader;

//----------------------------------------------------------------------------------
// Enums Definition
//----------------------------------------------------------------------------------

// Compression algorithms
// NOTE 1: This enum just list some common data compression algorithms for convenience,
// The rres packer tool and the engine-specific library are responsible to implement the desired ones,
// NOTE 2: rresResourceInfoHeader.compType is a byte-size value, limited to [0..255]
typedef enum rresCompressionType {
    RRES_COMP_NONE          = 0,            // No data compression
    RRES_COMP_RLE           = 1,            // RLE compression
    RRES_COMP_DEFLATE       = 10,           // DEFLATE compression
    RRES_COMP_LZ4           = 20,           // LZ4 compression
    RRES_COMP_LZMA2         = 30,           // LZMA2 compression
    RRES_COMP_QOI           = 40,           // QOI compression, useful for RGB(A) image data
    // gzip, brotli, lzo, zstd...           // TODO: Add other compression algorithms to enum
} rresCompressionType;

// Encryption algoritms
// NOTE 1: This enum just lists some common data encryption algorithms for convenience,
// The rres packer tool and the engine-specific library are responsible to implement the desired ones,
// NOTE 2: Some encryption algorithm could require/generate additional data (seed, salt, nonce, MAC...)
// in those cases, that extra data must be appended to the original encrypted message and added to the resource data chunk
// NOTE 3: rresResourceInfoHeader.cipherType is a byte-size value, limited to [0..255]
typedef enum rresEncryptionType {
    RRES_CIPHER_NONE        = 0,            // No data encryption
    RRES_CIPHER_XOR         = 1,            // XOR encryption, generic using 128bit key in blocks
    RRES_CIPHER_DES         = 10,           // DES encryption
    RRES_CIPHER_TDES        = 11,           // Triple DES encryption
    RRES_CIPHER_IDEA        = 20,           // IDEA encryption
    RRES_CIPHER_AES         = 30,           // AES (128bit or 256bit) encryption
    RRES_CIPHER_AES_GCM     = 31,           // AES Galois/Counter Mode (Galois Message Authentification Code - GMAC)
    RRES_CIPHER_XTEA        = 40,           // XTEA encryption
    RRES_CIPHER_BLOWFISH    = 50,           // BLOWFISH encryption
    RRES_CIPHER_RSA         = 60,           // RSA asymmetric encryption
    RRES_CIPHER_SALSA20     = 70,           // SALSA20 encryption
    RRES_CIPHER_CHACHA20    = 71,           // CHACHA20 encryption
    RRES_CIPHER_XCHACHA20   = 72,           // XCHACHA20 encryption
    RRES_CIPHER_XCHACHA20_POLY1305 = 73,    // XCHACHA20 with POLY1305 for message authentification (MAC) 
    // twofish, RC4, RC5, RC6               // TODO: Other encryption algorithm...
} rresEncryptionType;

// Image/Texture pixel formats
// NOTE: This enum list several common compressed/uncompressed pixel formats but there are actually MANY more
typedef enum rresPixelFormat {
    RRES_PIXELFORMAT_UNCOMP_GRAYSCALE = 1,  // 8 bit per pixel (no alpha)
    RRES_PIXELFORMAT_UNCOMP_GRAY_ALPHA,     // 16 bpp (2 channels)
    RRES_PIXELFORMAT_UNCOMP_R5G6B5,         // 16 bpp
    RRES_PIXELFORMAT_UNCOMP_R8G8B8,         // 24 bpp
    RRES_PIXELFORMAT_UNCOMP_R5G5B5A1,       // 16 bpp (1 bit alpha)
    RRES_PIXELFORMAT_UNCOMP_R4G4B4A4,       // 16 bpp (4 bit alpha)
    RRES_PIXELFORMAT_UNCOMP_R8G8B8A8,       // 32 bpp
    RRES_PIXELFORMAT_UNCOMP_R32,            // 32 bpp (1 channel - float)
    RRES_PIXELFORMAT_UNCOMP_R32G32B32,      // 32*3 bpp (3 channels - float)
    RRES_PIXELFORMAT_UNCOMP_R32G32B32A32,   // 32*4 bpp (4 channels - float)
    RRES_PIXELFORMAT_COMP_DXT1_RGB,         // 4 bpp (no alpha)
    RRES_PIXELFORMAT_COMP_DXT1_RGBA,        // 4 bpp (1 bit alpha)
    RRES_PIXELFORMAT_COMP_DXT3_RGBA,        // 8 bpp
    RRES_PIXELFORMAT_COMP_DXT5_RGBA,        // 8 bpp
    RRES_PIXELFORMAT_COMP_ETC1_RGB,         // 4 bpp
    RRES_PIXELFORMAT_COMP_ETC2_RGB,         // 4 bpp
    RRES_PIXELFORMAT_COMP_ETC2_EAC_RGBA,    // 8 bpp
    RRES_PIXELFORMAT_COMP_PVRT_RGB,         // 4 bpp
    RRES_PIXELFORMAT_COMP_PVRT_RGBA,        // 4 bpp
    RRES_PIXELFORMAT_COMP_ASTC_4x4_RGBA,    // 8 bpp
    RRES_PIXELFORMAT_COMP_ASTC_8x8_RGBA     // 2 bpp
    // TOO: Add additional pixel formats as required
} rresPixelFormat;

// Vertex data attribute
// NOTE: The expected number of components for every vertex attributes are the specified ones
typedef enum rresVertexAttribute {
    RRES_VERTEX_ATTRIBUTE_POSITION   = 0,   // Vertex position attribute: [x, y, z]
    RRES_VERTEX_ATTRIBUTE_TEXCOORD1  = 10,  // Vertex texture coordinates attribute: [u, v]
    RRES_VERTEX_ATTRIBUTE_TEXCOORD2  = 11,  // Vertex texture coordinates attribute: [u, v]
    RRES_VERTEX_ATTRIBUTE_TEXCOORD3  = 12,  // Vertex texture coordinates attribute: [u, v]
    RRES_VERTEX_ATTRIBUTE_TEXCOORD4  = 13,  // Vertex texture coordinates attribute: [u, v]
    RRES_VERTEX_ATTRIBUTE_NORMAL     = 20,  // Vertex normal attribute: [x, y, z]
    RRES_VERTEX_ATTRIBUTE_TANGENT    = 30,  // Vertex tangent attribute: [x, y, z, w]
    RRES_VERTEX_ATTRIBUTE_COLOR      = 40,  // Vertex color attribute: [r, g, b, a]
    RRES_VERTEX_ATTRIBUTE_INDEX      = 100, // Vertex index attribute: [i]
    // TODO: Add additional required attributes or attributes with different component count  
} rresVertexAttribute;

// Vertex data format type
typedef enum rresVertexFormat {
    RRES_VERTEX_FORMAT_UBYTE = 0,           // 8 bit unsigned integer data
    RRES_VERTEX_FORMAT_BYTE,                // 8 bit signed integer data
    RRES_VERTEX_FORMAT_USHORT,              // 16 bit unsigned integer data
    RRES_VERTEX_FORMAT_SHORT,               // 16 bit signed integer data
    RRES_VERTEX_FORMAT_UINT,                // 32 bit unsigned integer data
    RRES_VERTEX_FORMAT_INT,                 // 32 bit integer data
    RRES_VERTEX_FORMAT_HFLOAT,              // 16 bit float data
    RRES_VERTEX_FORMAT_FLOAT,               // 32 bit float data
    // TODO: Add additional required vertex formats (i.e. normalized data)
} rresVertexFormat;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
static rresResourceChunk rresLoadResourceChunk(rresResourceInfoHeader info, void *data);
static void rresUnloadResourceChunk(rresResourceChunk chunk);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Load resource from file by id
// NOTE: All resources conected to base id are loaded
rresResource rresLoadResource(const char *fileName, int rresId)
{
    rresResource rres = { 0 };

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) RRES_LOG("WARNING: [%s] rres file could not be opened\n", fileName);
    else
    {
        rresFileHeader header = { 0 };

        // Read rres file header
        fread(&header, sizeof(rresFileHeader), 1, rresFile);

        // Verify "rres" identifier
        if ((header.id[0] == 'r') &&
            (header.id[1] == 'R') &&
            (header.id[2] == 'E') &&
            (header.id[3] == 'S'))
        {
            for (int i = 0; i < header.chunkCount; i++)
            {
                rresResourceInfoHeader info = { 0 };

                // Read resource info header
                fread(&info, sizeof(rresResourceInfoHeader), 1, rresFile);

                if (info.id == rresId)
                {
                    rres.count = 1;

                    long currentFileOffset = ftell(rresFile);   // Store current file position
                    rresResourceInfoHeader temp = info;                 // Temp info header to scan resource chunks

                    // Scan all resource chunks checking temp.nextOffset
                    while (temp.nextOffset != 0)
                    {
                        fseek(rresFile, temp.nextOffset, SEEK_SET);         // Jump to next resource
                        fread(&temp, sizeof(rresResourceInfoHeader), 1, rresFile);  // Read next resource info header
                        rres.count++;
                    }

                    rres.chunks = (rresResourceChunk *)RRES_CALLOC(rres.count, sizeof(rresResourceChunk));   // Load as many rres slots as required
                    fseek(rresFile, currentFileOffset, SEEK_SET);           // Get to first resource chunk position

                    // Read and load data chunk from file data
                    void *data = RRES_MALLOC(info.packedSize);
                    fread(data, info.packedSize, 1, rresFile);
                    rres.chunks[0] = rresLoadResourceChunk(info, data);
                    RRES_FREE(data);

                    // Check if there are more chunks to read
                    if (rres.count > 1) RRES_LOG("INFO: [%s][ID %i] Multiple resource chunks detected: %i chunks\n", fileName, (int)info.id, rres.count);

                    int i = 0;

                    // Load all required resource chunks
                    while (info.nextOffset != 0)
                    {
                        void *data = RRES_MALLOC(info.packedSize);
                        fread(data, info.packedSize, 1, rresFile);

                        // Load chunk data from raw file data
                        // NOTE: Raw data can be compressed and encrypted,
                        // it can also contain data properties at the beginning
                        rres.chunks[i] = rresLoadResourceChunk(info, data);
                        RRES_FREE(data);

                        fseek(rresFile, temp.nextOffset, SEEK_SET);             // Jump to next resource
                        fread(&info, sizeof(rresResourceInfoHeader), 1, rresFile);      // Read next resource info header

                        i++;
                    }

                    break;      // Resource id found, stop checking the file
                }
                else
                {
                    // Skip required data size to read next resource infoHeader
                    fseek(rresFile, info.packedSize, SEEK_CUR);
                }
            }
        }

        fclose(rresFile);
    }

    return rres;
}

// Unload resource data
void rresUnloadResource(rresResource rres)
{
    for (unsigned int i = 0; i < rres.count; i++) rresUnloadResourceChunk(rres.chunks[i]);

    RRES_FREE(rres.chunks);
}

// Load central directory data
rresCentralDir rresLoadCentralDirectory(const char *fileName)
{
    rresCentralDir dir = { 0 };

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile != NULL)
    {
        rresFileHeader header = { 0 };

        fread(&header, sizeof(rresFileHeader), 1, rresFile);

        if ((header.id[0] == 'r') &&
            (header.id[1] == 'R') &&
            (header.id[2] == 'E') &&
            (header.id[3] == 'S') &&
            (header.cdOffset != 0))
        {
            RRES_LOG("WARNING: CDIR: Found at offset: %08x\n", header.cdOffset);

            rresResourceInfoHeader info = { 0 };

            fseek(rresFile, header.cdOffset, SEEK_CUR);         // Move to central directory position
            fread(&info, sizeof(rresResourceInfoHeader), 1, rresFile);  // Read resource info

            if ((info.type[0] == 'C') &&
                (info.type[1] == 'D') &&
                (info.type[2] == 'I') &&
                (info.type[3] == 'R'))
            {
                RRES_LOG("WARNING: CDIR: Found! Comp.Size: %i\n", info.packedSize);

                void *data = RRES_MALLOC(info.packedSize);
                fread(data, info.packedSize, 1, rresFile);

                rresResourceChunk chunk = rresLoadResourceChunk(info, data);    // TODO: Review: Only works for uncompressed/not-ecrypted data
                RRES_FREE(data);

                dir.count = chunk.props[0];     // Files count

                unsigned char *ptr = chunk.data;
                dir.entries = (rresDirEntry *)RRES_CALLOC(dir.count, sizeof(rresDirEntry));

                for (unsigned int i = 0; i < dir.count; i++)
                {
                    dir.entries[i].id = ((int *)ptr)[0];            // Resource id
                    dir.entries[i].offset = ((int*)ptr)[1];         // Resource offset in file
                    dir.entries[i].fileNameSize = ((int*)ptr)[2];   // Resource fileName size

                    // Resource fileName, NULL terminated, size considers that extra byte
                    memcpy(dir.entries[i].fileName, ptr + 16, dir.entries[i].fileNameSize);

                    ptr += (16 + dir.entries[i].fileNameSize);      // Move pointer for next entry
                }

                rresUnloadResourceChunk(chunk);
            }
        }

        fclose(rresFile);
    }

    return dir;
}

// Unload central directory data
void rresUnloadCentralDirectory(rresCentralDir dir)
{
    RRES_FREE(dir.entries);
}

// Get resource identifier from filename
// WARNING: It requires the central directory previously loaded
int rresGetIdFromFileName(rresCentralDir dir, const char *fileName)
{
    int id = 0;

    for (unsigned int i = 0; i < dir.count; i++)
    {
        // NOTE: entries[i].fileName is NULL terminated

        if (strcmp((const char *)dir.entries[i].fileName, fileName) == 0)
        {
            id = dir.entries[i].id;
            break;
        }
    }

    return id;
}

// Compute CRC32 hash
// NOTE: CRC32 is used as rres id, generated from original filename
unsigned int rresComputeCRC32(unsigned char *buffer, int len)
{
    static unsigned int crcTable[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0eDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    unsigned int crc = ~0u;

    for (int i = 0; i < len; i++) crc = (crc >> 8)^crcTable[buffer[i]^(crc&0xff)];

    return ~crc;
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------
// Load data chunk from raw resource data as contained in file
// WARNING: Raw data can be compressed and/or encrypted, in those cases is up to the users to load it
static rresResourceChunk rresLoadResourceChunk(rresResourceInfoHeader info, void *data)
{
    rresResourceChunk chunk = { 0 };

    // Assign rres.type (int) from info.type (FourCC)
    if ((info.type[0] == 'N') && (info.type[1] == 'U') && (info.type[2] == 'L') && (info.type[3] == 'L')) chunk.type = RRES_DATA_NULL;            // NULL
    if ((info.type[0] == 'R') && (info.type[1] == 'A') && (info.type[2] == 'W') && (info.type[3] == 'D')) chunk.type = RRES_DATA_RAW;             // RAWD
    else if ((info.type[0] == 'T') && (info.type[1] == 'E') && (info.type[2] == 'X') && (info.type[3] == 'T')) chunk.type = RRES_DATA_TEXT;       // TEXT
    else if ((info.type[0] == 'I') && (info.type[1] == 'M') && (info.type[2] == 'G') && (info.type[3] == 'E')) chunk.type = RRES_DATA_IMAGE;      // IMGE
    else if ((info.type[0] == 'W') && (info.type[1] == 'A') && (info.type[2] == 'V') && (info.type[3] == 'E')) chunk.type = RRES_DATA_WAVE;       // WAVE
    else if ((info.type[0] == 'V') && (info.type[1] == 'R') && (info.type[2] == 'T') && (info.type[3] == 'X')) chunk.type = RRES_DATA_VERTEX;     // VRTX
    else if ((info.type[0] == 'F') && (info.type[1] == 'N') && (info.type[2] == 'T') && (info.type[3] == 'G')) chunk.type = RRES_DATA_GLYPH_INFO; // FNTG
    else if ((info.type[0] == 'L') && (info.type[1] == 'I') && (info.type[2] == 'N') && (info.type[3] == 'K')) chunk.type = RRES_DATA_LINK;       // LINK
    else if ((info.type[0] == 'C') && (info.type[1] == 'D') && (info.type[2] == 'I') && (info.type[3] == 'R')) chunk.type = RRES_DATA_DIRECTORY;  // CDIR

    if (chunk.type != RRES_DATA_NULL)   // Make sure chunk contains data (or is supposed to)
    {
        // Check that data chunk is not compressed or encrypted to retrieve [properties + data]
        if ((info.compType == RRES_COMP_NONE) && (info.cipherType == RRES_CIPHER_NONE))
        {
            // NOTE: If data is not compressed/encrypted info.size = info.baseSize

            unsigned int crc32 = rresComputeCRC32(data, info.packedSize);   // CRC32 data validation

            if (crc32 == info.crc32)    // Check CRC32
            {
                chunk.propCount = ((int *)data)[0];

                if (chunk.propCount > 0)
                {
                    chunk.props = (int *)RRES_CALLOC(chunk.propCount, sizeof(int));
                    for (unsigned int i = 0; i < chunk.propCount; i++) chunk.props[i] = ((int *)data)[1 + i];
                }

                chunk.data = RRES_MALLOC(info.baseSize);
                memcpy(chunk.data, ((unsigned char *)data) + sizeof(int) + (chunk.propCount*sizeof(int)), info.baseSize);
            }
            else
            {
                RRES_LOG("WARNING: [ID %i] CRC32 does not match, data can be corrupted\n", info.id);
            }
        }
        else
        {
            chunk.compType = info.compType;
            chunk.cipherType = info.cipherType;
        }
    }

    return chunk;
}

// Unload data chunk
static void rresUnloadResourceChunk(rresResourceChunk chunk)
{
    RRES_FREE(chunk.props);   // Resource chunk properties
    RRES_FREE(chunk.data);    // Resource chunk data
}

#endif // RRES_IMPLEMENTATION
