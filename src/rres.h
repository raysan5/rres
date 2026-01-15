/**********************************************************************************************
*
*   rres v1.0 - A simple and easy-to-use file-format to package resources
*
*   CONFIGURATION:
*
*   #define RRES_IMPLEMENTATION
*       Generates the implementation of the library into the included file
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation
*
*   FEATURES:
*
*     - Multi-resource files: Some files could end-up generating multiple connected resources in
*       the rres output file (i.e TTF files could generate RRES_DATA_FONT_GLYPHS and RRES_DATA_IMAGE)
*     - File packaging as raw resource data: Avoid data processing and just package the file bytes
*     - Per-file data compression/encryption: Configure compression/encription for every input file
*     - Externally linked files: Package only the file path, to be loaded from external file when the
*       specific id is requested. WARNING: Be careful with path, it should be relative to application dir
*     - Central Directory resource (optional): Create a central directory with the input filename relation
*       to the resource(s) id. This is the default option but it can be avoided; in that case, a header
*       file (.h) is generated with the file ids definitions
*
*   FILE STRUCTURE:
*
*   rres files consist of a file header followed by a number of resource chunks
*
*   Optionally it can contain a Central Directory resource chunk (usually at the end) with the info
*   of all the files processed into the rres file.
*
*   NOTE: Chunks count could not match files count, some processed files (i.e Font, Mesh)
*   could generate multiple chunks with the same id related by the rresResourceChunkInfo.nextOffset
*   Those chunks are loaded together when resource is loaded
*
*   rresFileHeader               (16 bytes)
*       Signature Id              (4 bytes)     // File signature id: 'rres'
*       Version                   (2 bytes)     // Format version
*       Resource Count            (2 bytes)     // Number of resource chunks contained
*       CD Offset                 (4 bytes)     // Central Directory offset (if available)
*       Reserved                  (4 bytes)     // <reserved>
*
*   rresResourceChunk[]
*   {
*       rresResourceChunkInfo   (32 bytes)
*           Type                  (4 bytes)     // Resource type (FourCC)
*           Id                    (4 bytes)     // Resource identifier (CRC32 filename hash or custom)
*           Compressor            (1 byte)      // Data compression algorithm
*           Cipher                (1 byte)      // Data encryption algorithm
*           Flags                 (2 bytes)     // Data flags (if required)
*           Data Packed Size      (4 bytes)     // Data packed size (compressed/encrypted + custom data appended)
*           Data Base Size        (4 bytes)     // Data base size (uncompressed/unencrypted)
*           Next Offset           (4 bytes)     // Next resource chunk offset (if required)
*           Reserved              (4 bytes)     // <reserved>
*           CRC32                 (4 bytes)     // Resource Data Chunk CRC32
*
*       rresResourceChunkData     (n bytes)     // Packed data
*           Property Count        (4 bytes)     // Number of properties contained
*           Properties[]          (4*i bytes)   // Resource data required properties, depend on Type
*           Data                  (m bytes)     // Resource data
*   }
*
*   rresResourceChunk: RRES_DATA_DIRECTORY      // Central directory (special resource chunk)
*   {
*       rresResourceChunkInfo   (32 bytes)
*
*       rresCentralDir            (n bytes)     // rresResourceChunkData
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
*   DESIGN DECISIONS / LIMITATIONS:
*
*     - rres file maximum chunks: 65535 (16bit chunk count in rresFileHeader)
*     - rres file maximum size: 4GB (chunk offset and Central Directory Offset is 32bit, so it can not address more than 4GB
*     - Chunk search by ID is done one by one, starting at first chunk and accessed with fread() function
*     - Endianness: rres does not care about endianness, data is stored as desired by the host platform (most probably Little Endian)
*       Endianness won't affect chunk data but it will affect rresFileHeader and rresResourceChunkInfo
*     - CRC32 hash is used to to generate the rres file identifier from filename
*       There is a "small" probability of random collision (1 in 2^32 approx.) but considering
*       the chance of collision is related to the number of data inputs, not the size of the inputs, we assume that risk
*       Also note that CRC32 is not used as a security/cryptographic hash, just an identifier for the input file
*     - CRC32 hash is also used to detect chunk data corruption. CRC32 is smaller and computationally much less complex than MD5 or SHA1.
*       Using a hash function like MD5 is probably overkill for random error detection
*     - Central Directory rresDirEntry.fileName is NULL terminated and padded to 4-byte, rresDirEntry.fileNameSize considers the padding
*     - Compression and Encryption. rres supports chunks data compression and encryption, it provides two fields in the rresResourceChunkInfo to
*       note it, but in those cases is up to the user to implement the desired compressor/uncompressor and encryption/decryption mechanisms
*       In case of data encryption, it's recommended that any additional resource data (i.e. MAC) to be appended to data chunk and properly
*       noted in the packed data size field of rresResourceChunkInfo. Data compression should be applied before encryption
*
*   DEPENDENCIES:
*
*   rres library dependencies has been keep to the minimum. It depends only some libc functionality:
*
*     - stdlib.h: Required for memory allocation: malloc(), calloc(), free()
*                 NOTE: Allocators can be redefined with macros RRES_MALLOC, RRES_CALLOC, RRES_FREE
*     - stdio.h:  Required for file access functionality: FILE, fopen(), fseek(), fread(), fclose()
*     - string.h: Required for memory data management: memcpy(), memcmp()
*
*   VERSION HISTORY:
*
*     - 1.0 (12-May-2022): Implementation review for better alignment with rres specs
*     - 0.9 (28-Apr-2022): Initial implementation of rres specs
*
*
*   LICENSE: MIT
*
*   Copyright (c) 2016-2026 Ramon Santamaria (@raysan5)
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

// On Windows, MAX_PATH is limited to 256 by default,
// on Linux, it could go up to 4096
#define RRES_MAX_FILENAME_SIZE      1024

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// rres file header (16 bytes)
typedef struct rresFileHeader {
    unsigned char id[4];            // File identifier: rres
    unsigned short version;         // File version: 100 for version 1.0
    unsigned short chunkCount;      // Number of resource chunks in the file (MAX: 65535)
    unsigned int cdOffset;          // Central Directory offset in file (0 if not available)
    unsigned int reserved;          // <reserved>
} rresFileHeader;

// rres resource chunk info header (32 bytes)
typedef struct rresResourceChunkInfo {
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
} rresResourceChunkInfo;

// rres resource chunk data
typedef struct rresResourceChunkData {
    unsigned int propCount;         // Resource chunk properties count
    unsigned int *props;            // Resource chunk properties
    void *raw;                      // Resource chunk raw data
} rresResourceChunkData;

// rres resource chunk
typedef struct rresResourceChunk {
    rresResourceChunkInfo info;     // Resource chunk info
    rresResourceChunkData data;     // Resource chunk packed data, contains propCount, props[] and raw data
} rresResourceChunk;

// rres resource multi
// NOTE: It supports multiple resource chunks
typedef struct rresResourceMulti {
    unsigned int count;             // Resource chunks count
    rresResourceChunk *chunks;      // Resource chunks
} rresResourceMulti;

// Useful data types for specific chunk types
//----------------------------------------------------------------------
// CDIR: rres central directory entry
typedef struct rresDirEntry {
    unsigned int id;                // Resource id
    unsigned int offset;            // Resource global offset in file
    unsigned int reserved;          // reserved
    unsigned int fileNameSize;      // Resource fileName size (NULL terminator and 4-byte alignment padding considered)
    char fileName[RRES_MAX_FILENAME_SIZE];  // Resource original fileName (NULL terminated and padded to 4-byte alignment)
} rresDirEntry;

// CDIR: rres central directory
// NOTE: This data conforms the rresResourceChunkData
typedef struct rresCentralDir {
    unsigned int count;             // Central directory entries count
    rresDirEntry *entries;          // Central directory entries
} rresCentralDir;

// FNTG: rres font glyphs info (32 bytes)
// NOTE: And array of this type conforms the rresResourceChunkData
typedef struct rresFontGlyphInfo {
    int x;                          // Glyph rectangle X in the atlas image
    int y;                          // Glyph rectangle Y in the atlas image
    int width;                      // Glyph rectangle width in the atlas image
    int height;                     // Glyph rectangle height in the atlas image
    int value;                      // Glyph codepoint value
    int offsetX;                    // Glyph drawing offset X (from base line)
    int offsetY;                    // Glyph drawing offset Y (from base line)
    int advanceX;                   // Glyph advance X for next character
} rresFontGlyphInfo;

//----------------------------------------------------------------------------------
// Enums Definition
// The following enums are useful to fill some fields of the rresResourceChunkInfo
// and also some fields of the different data types properties
//----------------------------------------------------------------------------------

// rres resource chunk data type
// NOTE 1: Data type determines the properties and the data included in every chunk
// NOTE 2: This enum defines the basic resource data types,
// some input files could generate multiple resource chunks:
//   Fonts processed could generate (2) resource chunks:
//   - [FNTG] rres[0]: RRES_DATA_FONT_GLYPHS
//   - [IMGE] rres[1]: RRES_DATA_IMAGE
//
//   Mesh processed could generate (n) resource chunks:
//   - [VRTX] rres[0]: RRES_DATA_VERTEX
//   ...
//   - [VRTX] rres[n]: RRES_DATA_VERTEX
typedef enum rresResourceDataType {
    RRES_DATA_NULL         = 0,             // FourCC: NULL - Reserved for empty chunks, no props/data
    RRES_DATA_RAW          = 1,             // FourCC: RAWD - Raw file data, 4 properties
                                            //    props[0]:size (bytes)
                                            //    props[1]:extension01 (big-endian: ".png" = 0x2e706e67)
                                            //    props[2]:extension02 (additional part, extensions with +3 letters)
                                            //    props[3]:reserved
                                            //    data: raw bytes
    RRES_DATA_TEXT         = 2,             // FourCC: TEXT - Text file data, 4 properties
                                            //    props[0]:size (bytes)
                                            //    props[1]:rresTextEncoding
                                            //    props[2]:rresCodeLang
                                            //    props[3]:cultureCode
                                            //    data: text
    RRES_DATA_IMAGE        = 3,             // FourCC: IMGE - Image file data, 4 properties
                                            //    props[0]:width
                                            //    props[1]:height
                                            //    props[2]:rresPixelFormat
                                            //    props[3]:mipmaps
                                            //    data: pixels
    RRES_DATA_WAVE         = 4,             // FourCC: WAVE - Audio file data, 4 properties
                                            //    props[0]:frameCount
                                            //    props[1]:sampleRate
                                            //    props[2]:sampleSize
                                            //    props[3]:channels
                                            //    data: samples
    RRES_DATA_VERTEX       = 5,             // FourCC: VRTX - Vertex file data, 4 properties
                                            //    props[0]:vertexCount
                                            //    props[1]:rresVertexAttribute
                                            //    props[2]:componentCount
                                            //    props[3]:rresVertexFormat
                                            //    data: vertex
    RRES_DATA_FONT_GLYPHS  = 6,             // FourCC: FNTG - Font glyphs info data, 4 properties
                                            //    props[0]:baseSize
                                            //    props[1]:glyphCount
                                            //    props[2]:glyphPadding
                                            //    props[3]:rresFontStyle
                                            //    data: rresFontGlyphInfo[0..glyphCount]
    RRES_DATA_LINK         = 99,            // FourCC: LINK - External linked file, 1 property
                                            //    props[0]:size (bytes)
                                            //    data: filepath (as provided on input)
    RRES_DATA_DIRECTORY    = 100,           // FourCC: CDIR - Central directory for input files
                                            //    props[0]:entryCount, 1 property
                                            //    data: rresDirEntry[0..entryCount]

    // TODO: 2.0: Support resource package types (muti-resource)
    // NOTE: They contains multiple rresResourceChunk in rresResourceData.raw
    //RRES_DATA_PACK_FONT    = 110,         // FourCC: PFNT - Resources Pack: Font data, 1 property (2 resource chunks: RRES_DATA_GLYPHS, RRES_DATA_IMAGE)
                                            //    props[0]:chunkCount
    //RRES_DATA_PACK_MESH    = 120,         // FourCC: PMSH - Resources Pack: Mesh data, 1 property (n resource chunks: RRES_DATA_VERTEX)
                                            //    props[0]:chunkCount

    // TODO: Add additional resource data types if required (define props + data)

} rresResourceDataType;

// Compression algorithms
// Value required by rresResourceChunkInfo.compType
// NOTE 1: This enum just list some common data compression algorithms for convenience,
// The rres packer tool and the engine-specific library are responsible to implement the desired ones,
// NOTE 2: rresResourceChunkInfo.compType is a byte-size value, limited to [0..255]
typedef enum rresCompressionType {
    RRES_COMP_NONE          = 0,            // No data compression
    RRES_COMP_RLE           = 1,            // RLE compression
    RRES_COMP_DEFLATE       = 10,           // DEFLATE compression
    RRES_COMP_LZ4           = 20,           // LZ4 compression
    RRES_COMP_LZMA2         = 30,           // LZMA2 compression
    RRES_COMP_QOI           = 40,           // QOI compression, useful for RGB(A) image data
    // TODO: Add additional compression algorithms if required
} rresCompressionType;

// Encryption algoritms
// Value required by rresResourceChunkInfo.cipherType
// NOTE 1: This enum just lists some common data encryption algorithms for convenience,
// The rres packer tool and the engine-specific library are responsible to implement the desired ones,
// NOTE 2: Some encryption algorithm could require/generate additional data (seed, salt, nonce, MAC...)
// in those cases, that extra data must be appended to the original encrypted message and added to the resource data chunk
// NOTE 3: rresResourceChunkInfo.cipherType is a byte-size value, limited to [0..255]
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
    // TODO: Add additional encryption algorithm if required
} rresEncryptionType;

// TODO: rres error codes (not used at this moment)
// NOTE: Error codes when processing rres files
typedef enum rresErrorType {
    RRES_SUCCESS = 0,                       // rres file loaded/saved successfully
    RRES_ERROR_FILE_NOT_FOUND,              // rres file can not be opened (spelling issues, file actually does not exist...)
    RRES_ERROR_FILE_FORMAT,                 // rres file format not a supported (wrong header, wrong identifier)
    RRES_ERROR_MEMORY_ALLOC,                // Memory could not be allocated for operation
} rresErrorType;

// Enums required by specific resource types for its properties
//----------------------------------------------------------------------------------
// TEXT: Text encoding property values
typedef enum rresTextEncoding {
    RRES_TEXT_ENCODING_UNDEFINED = 0,       // Not defined, usually UTF-8
    RRES_TEXT_ENCODING_UTF8      = 1,       // UTF-8 text encoding
    RRES_TEXT_ENCODING_UTF8_BOM  = 2,       // UTF-8 text encoding with Byte-Order-Mark
    RRES_TEXT_ENCODING_UTF16_LE  = 10,      // UTF-16 Little Endian text encoding
    RRES_TEXT_ENCODING_UTF16_BE  = 11,      // UTF-16 Big Endian text encoding
    // TODO: Add additional encodings if required
} rresTextEncoding;

// TEXT: Text code language
// NOTE: It could be useful for code script resources
typedef enum rresCodeLang {
    RRES_CODE_LANG_UNDEFINED = 0,           // Undefined code language, text is plain text
    RRES_CODE_LANG_C,                       // Text contains C code
    RRES_CODE_LANG_CPP,                     // Text contains C++ code
    RRES_CODE_LANG_CS,                      // Text contains C# code
    RRES_CODE_LANG_LUA,                     // Text contains Lua code
    RRES_CODE_LANG_JS,                      // Text contains JavaScript code
    RRES_CODE_LANG_PYTHON,                  // Text contains Python code
    RRES_CODE_LANG_RUST,                    // Text contains Rust code
    RRES_CODE_LANG_ZIG,                     // Text contains Zig code
    RRES_CODE_LANG_ODIN,                    // Text contains Odin code
    RRES_CODE_LANG_JAI,                     // Text contains Jai code
    RRES_CODE_LANG_GDSCRIPT,                // Text contains GDScript (Godot) code
    RRES_CODE_LANG_GLSL,                    // Text contains GLSL shader code
    // TODO: Add additional code languages if required
} rresCodeLang;

// IMGE: Image/Texture pixel formats
typedef enum rresPixelFormat {
    RRES_PIXELFORMAT_UNDEFINED = 0,
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
    // TOO: Add additional pixel formats if required
} rresPixelFormat;

// VRTX: Vertex data attribute
// NOTE: The expected number of components for every vertex attributes is provided as a property to data,
// the listed components count are the expected/default ones
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
    // TODO: Add additional attributes if required
} rresVertexAttribute;

// VRTX: Vertex data format type
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

// FNTG: Font style
typedef enum rresFontStyle {
    RRES_FONT_STYLE_UNDEFINED = 0,          // Undefined font style
    RRES_FONT_STYLE_REGULAR,                // Regular font style
    RRES_FONT_STYLE_BOLD,                   // Bold font style
    RRES_FONT_STYLE_ITALIC,                 // Italic font style
    // TODO: Add additional font styles if required
} rresFontStyle;

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

// Load only one resource chunk (first resource id found)
RRESAPI rresResourceChunk rresLoadResourceChunk(const char *fileName, unsigned int rresId);  // Load one resource chunk for provided id
RRESAPI void rresUnloadResourceChunk(rresResourceChunk chunk);                      // Unload resource chunk from memory

// Load multi resource chunks for a specified rresId
RRESAPI rresResourceMulti rresLoadResourceMulti(const char *fileName, unsigned int rresId);  // Load resource for provided id (multiple resource chunks)
RRESAPI void rresUnloadResourceMulti(rresResourceMulti multi);                      // Unload resource from memory (multiple resource chunks)

// Load resource(s) chunk info from file
RRESAPI rresResourceChunkInfo rresLoadResourceChunkInfo(const char *fileName, unsigned int rresId);  // Load resource chunk info for provided id
RRESAPI rresResourceChunkInfo *rresLoadResourceChunkInfoAll(const char *fileName, unsigned int *chunkCount); // Load all resource chunks info

RRESAPI rresCentralDir rresLoadCentralDirectory(const char *fileName);              // Load central directory resource chunk from file
RRESAPI void rresUnloadCentralDirectory(rresCentralDir dir);                        // Unload central directory resource chunk

RRESAPI unsigned int rresGetDataType(const unsigned char *fourCC);                  // Get rresResourceDataType from FourCC code
RRESAPI unsigned int rresGetResourceId(rresCentralDir dir, const char *fileName);            // Get resource id for a provided filename
                                                                                    // NOTE: It requires CDIR available in the file (it's optinal by design)
RRESAPI unsigned int rresComputeCRC32(const unsigned char *data, int len);          // Compute CRC32 for provided data

// Manage password for data encryption/decryption
// NOTE: The cipher password is kept as an internal pointer to provided string, it's up to the user to manage that sensible data properly
// Password should be to allocate and set before loading an encrypted resource and it should be cleaned/wiped after the encrypted resource has been loaded
// TODO: Move this functionality to engine-library, after all rres.h does not manage data decryption
RRESAPI void rresSetCipherPassword(const char *pass);                 // Set password to be used on data decryption
RRESAPI const char *rresGetCipherPassword(void);                      // Get password to be used on data decryption

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

// Boolean type
#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool)
    typedef enum bool { false = 0, true = !false } bool;
    #define RL_BOOL_TYPE
#endif

#include <stdlib.h>                 // Required for: malloc(), calloc(), free()
#include <stdio.h>                  // Required for: FILE, fopen(), fseek(), fread(), fclose()
#include <string.h>                 // Required for: memcpy(), memcmp()

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char *password = NULL;     // Password pointer, managed by user libraries

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
// Load resource chunk packed data into our data struct
static rresResourceChunkData rresLoadResourceChunkData(rresResourceChunkInfo info, void *packedData);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Load one resource chunk for provided id
rresResourceChunk rresLoadResourceChunk(const char *fileName, unsigned int rresId)
{
    rresResourceChunk chunk = { 0 };

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) RRES_LOG("RRES: WARNING: [%s] rres file could not be opened\n", fileName);
    else
    {
        RRES_LOG("RRES: INFO: Loading resource from file: %s\n", fileName);

        rresFileHeader header = { 0 };

        // Read rres file header
        fread(&header, sizeof(rresFileHeader), 1, rresFile);

        // Verify file signature: "rres" and file version: 100
        if (((header.id[0] == 'r') && (header.id[1] == 'r') && (header.id[2] == 'e') && (header.id[3] == 's')) && (header.version == 100))
        {
            bool found = false;

            // Check all available chunks looking for the requested id
            for (int i = 0; i < header.chunkCount; i++)
            {
                rresResourceChunkInfo info = { 0 };

                // Read resource info header
                fread(&info, sizeof(rresResourceChunkInfo), 1, rresFile);

                // Check if resource id is the requested one
                if (info.id == rresId)
                {
                    found = true;

                    RRES_LOG("RRES: INFO: Found requested resource id: 0x%08x\n", info.id);
                    RRES_LOG("RRES: %c%c%c%c: Id: 0x%08x | Base size: %i | Packed size: %i\n", info.type[0], info.type[1], info.type[2], info.type[3], info.id, info.baseSize, info.packedSize);

                    // NOTE: We only load first matching id resource chunk found but
                    // we show a message if additional chunks are detected
                    if (info.nextOffset != 0) RRES_LOG("RRES: WARNING: Multiple linked resource chunks available for the provided id");

                    /*
                    // Variables required to check multiple chunks
                    int chunkCount = 0;
                    long currentFileOffset = ftell(rresFile);           // Store current file position
                    rresResourceChunkInfo temp = info;                  // Temp info header to scan resource chunks

                    // Count all linked resource chunks checking temp.nextOffset
                    while (temp.nextOffset != 0)
                    {
                        fseek(rresFile, temp.nextOffset, SEEK_SET);     // Jump to next linked resource
                        fread(&temp, sizeof(rresResourceChunkInfo), 1, rresFile);  // Read next resource info header
                        chunkCount++;
                    }

                    fseek(rresFile, currentFileOffset, SEEK_SET);       // Return to first resource chunk position
                    */

                    // Read and resource chunk from file data
                    // NOTE: Read data can be compressed/encrypted, it's up to the user library to manage decompression/decryption
                    void *data = RRES_CALLOC(info.packedSize, 1); // Allocate enough memory to store resource data chunk
                    fread(data, info.packedSize, 1, rresFile);    // Read data: propsCount + props[] + data (+additional_data)

                    // Get chunk.data properly organized (only if uncompressed/unencrypted)
                    chunk.data = rresLoadResourceChunkData(info, data);
                    chunk.info = info;

                    RRES_FREE(data);

                    break;      // Resource id found and loaded, stop checking the file
                }
                else
                {
                    // Skip required data size to read next resource info header
                    fseek(rresFile, info.packedSize, SEEK_CUR);
                }
            }

            if (!found) RRES_LOG("RRES: WARNING: Requested resource not found: 0x%08x\n", rresId);
        }
        else RRES_LOG("RRES: WARNING: The provided file is not a valid rres file, file signature or version not valid\n");

        fclose(rresFile);
    }

    return chunk;
}

// Unload resource chunk from memory
void rresUnloadResourceChunk(rresResourceChunk chunk)
{
    RRES_FREE(chunk.data.props);  // Resource chunk properties
    RRES_FREE(chunk.data.raw);    // Resource chunk raw data
}

// Load resource from file by id
// NOTE: All resources conected to base id are loaded
rresResourceMulti rresLoadResourceMulti(const char *fileName, unsigned int rresId)
{
    rresResourceMulti rres = { 0 };

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) RRES_LOG("RRES: WARNING: [%s] rres file could not be opened\n", fileName);
    else
    {
        rresFileHeader header = { 0 };

        // Read rres file header
        fread(&header, sizeof(rresFileHeader), 1, rresFile);

        // Verify file signature: "rres" and file version: 100
        if (((header.id[0] == 'r') && (header.id[1] == 'r') && (header.id[2] == 'e') && (header.id[3] == 's')) && (header.version == 100))
        {
            bool found = false;

            // Check all available chunks looking for the requested id
            for (int i = 0; i < header.chunkCount; i++)
            {
                rresResourceChunkInfo info = { 0 };

                // Read resource info header
                fread(&info, sizeof(rresResourceChunkInfo), 1, rresFile);

                // Check if resource id is the requested one
                if (info.id == rresId)
                {
                    found = true;

                    RRES_LOG("RRES: INFO: Found requested resource id: 0x%08x\n", info.id);
                    RRES_LOG("RRES: %c%c%c%c: Id: 0x%08x | Base size: %i | Packed size: %i\n", info.type[0], info.type[1], info.type[2], info.type[3], info.id, info.baseSize, info.packedSize);

                    rres.count = 1;

                    long currentFileOffset = ftell(rresFile);               // Store current file position
                    rresResourceChunkInfo temp = info;                      // Temp info header to scan resource chunks

                    // Count all linked resource chunks checking temp.nextOffset
                    while (temp.nextOffset != 0)
                    {
                        fseek(rresFile, temp.nextOffset, SEEK_SET);         // Jump to next linked resource
                        fread(&temp, sizeof(rresResourceChunkInfo), 1, rresFile); // Read next resource info header
                        rres.count++;
                    }

                    rres.chunks = (rresResourceChunk *)RRES_CALLOC(rres.count, sizeof(rresResourceChunk)); // Load as many rres slots as required
                    fseek(rresFile, currentFileOffset, SEEK_SET);           // Return to first resource chunk position

                    // Read and load data chunk from file data
                    // NOTE: Read data can be compressed/encrypted,
                    // it's up to the user library to manage decompression/decryption
                    void *data = RRES_CALLOC(info.packedSize, 1);           // Allocate enough memory to store resource data chunk
                    fread(data, info.packedSize, 1, rresFile);              // Read data: propsCount + props[] + data (+additional_data)

                    // Get chunk.data properly organized (only if uncompressed/unencrypted)
                    rres.chunks[0].data = rresLoadResourceChunkData(info, data);
                    rres.chunks[0].info = info;

                    RRES_FREE(data);

                    int i = 1;

                    // Load all linked resource chunks
                    while (info.nextOffset != 0)
                    {
                        fseek(rresFile, info.nextOffset, SEEK_SET);         // Jump to next resource chunk
                        fread(&info, sizeof(rresResourceChunkInfo), 1, rresFile); // Read next resource info header

                        RRES_LOG("RRES: %c%c%c%c: Id: 0x%08x | Base size: %i | Packed size: %i\n", info.type[0], info.type[1], info.type[2], info.type[3], info.id, info.baseSize, info.packedSize);

                        void *data = RRES_CALLOC(info.packedSize, 1);       // Allocate enough memory to store resource data chunk
                        fread(data, info.packedSize, 1, rresFile);          // Read data: propsCount + props[] + data (+additional_data)

                        // Get chunk.data properly organized (only if uncompressed/unencrypted)
                        rres.chunks[i].data = rresLoadResourceChunkData(info, data);
                        rres.chunks[i].info = info;

                        RRES_FREE(data);

                        i++;
                    }

                    break;      // Resource id found and loaded, stop checking the file
                }
                else
                {
                    // Skip required data size to read next resource info header
                    fseek(rresFile, info.packedSize, SEEK_CUR);
                }
            }

            if (!found) RRES_LOG("RRES: WARNING: Requested resource not found: 0x%08x\n", rresId);
        }
        else RRES_LOG("RRES: WARNING: The provided file is not a valid rres file, file signature or version not valid\n");

        fclose(rresFile);
    }

    return rres;
}

// Unload resource data
void rresUnloadResourceMulti(rresResourceMulti multi)
{
    for (unsigned int i = 0; i < multi.count; i++) rresUnloadResourceChunk(multi.chunks[i]);

    RRES_FREE(multi.chunks);
}

// Load resource chunk info for provided id
RRESAPI rresResourceChunkInfo rresLoadResourceChunkInfo(const char *fileName, unsigned int rresId)
{
    rresResourceChunkInfo info = { 0 };

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile != NULL)
    {
        rresFileHeader header = { 0 };

        fread(&header, sizeof(rresFileHeader), 1, rresFile);

        // Verify file signature: "rres", file version: 100
        if (((header.id[0] == 'r') && (header.id[1] == 'r') && (header.id[2] == 'e') && (header.id[3] == 's')) && (header.version == 100))
        {
            // Try to find provided resource chunk id and read info chunk
            for (int i = 0; i < header.chunkCount; i++)
            {
                // Read resource chunk info
                fread(&info, sizeof(rresResourceChunkInfo), 1, rresFile);

                if (info.id == rresId)
                {
                    // TODO: Jump to next resource chunk for provided id
                    //if (info.nextOffset > 0) fseek(rresFile, info.nextOffset, SEEK_SET);

                    break; // If requested rresId is found, we return the read rresResourceChunkInfo
                }
                else fseek(rresFile, info.packedSize, SEEK_CUR); // Jump to next resource
            }
        }
        else RRES_LOG("RRES: WARNING: The provided file is not a valid rres file, file signature or version not valid\n");

        fclose(rresFile);
    }

    return info;
}

// Load all resource chunks info
RRESAPI rresResourceChunkInfo *rresLoadResourceChunkInfoAll(const char *fileName, unsigned int *chunkCount)
{
    rresResourceChunkInfo *infos = { 0 };
    unsigned int count = 0;

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile != NULL)
    {
        rresFileHeader header = { 0 };

        fread(&header, sizeof(rresFileHeader), 1, rresFile);

        // Verify file signature: "rres", file version: 100
        if (((header.id[0] == 'r') && (header.id[1] == 'r') && (header.id[2] == 'e') && (header.id[3] == 's')) && (header.version == 100))
        {
            // Load all resource chunks info
            infos = (rresResourceChunkInfo *)RRES_CALLOC(header.chunkCount, sizeof(rresResourceChunkInfo));
            count = header.chunkCount;

            for (unsigned int i = 0; i < count; i++)
            {
                fread(&infos[i], sizeof(rresResourceChunkInfo), 1, rresFile); // Read resource chunk info

                if (infos[i].nextOffset > 0) fseek(rresFile, infos[i].nextOffset, SEEK_SET); // Jump to next resource
                else fseek(rresFile, infos[i].packedSize, SEEK_CUR); // Jump to next resource
            }
        }
        else RRES_LOG("RRES: WARNING: The provided file is not a valid rres file, file signature or version not valid\n");

        fclose(rresFile);
    }

    *chunkCount = count;
    return infos;
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

        // Verify file signature: "rres", file version: 100
        if (((header.id[0] == 'r') && (header.id[1] == 'r') && (header.id[2] == 'e') && (header.id[3] == 's')) && (header.version == 100))
        {
            // Check if there is a Central Directory available
            if (header.cdOffset == 0) RRES_LOG("RRES: WARNING: CDIR: No central directory found\n");
            else
            {
                rresResourceChunkInfo info = { 0 };

                fseek(rresFile, header.cdOffset, SEEK_CUR); // Move to central directory position
                fread(&info, sizeof(rresResourceChunkInfo), 1, rresFile); // Read resource info

                // Verify resource type is CDIR
                if ((info.type[0] == 'C') && (info.type[1] == 'D') && (info.type[2] == 'I') && (info.type[3] == 'R'))
                {
                    RRES_LOG("RRES: CDIR: Central Directory found at offset: 0x%08x\n", header.cdOffset);

                    void *data = RRES_CALLOC(info.packedSize, 1);
                    fread(data, info.packedSize, 1, rresFile);

                    // Load resource chunk data (central directory), data is uncompressed/unencrypted by default
                    rresResourceChunkData chunkData = rresLoadResourceChunkData(info, data);
                    RRES_FREE(data);

                    dir.count = chunkData.props[0];     // File entries count

                    RRES_LOG("RRES: CDIR: Central Directory file entries count: %i\n", dir.count);

                    unsigned char *ptr = (unsigned char *)chunkData.raw;
                    dir.entries = (rresDirEntry *)RRES_CALLOC(dir.count, sizeof(rresDirEntry));

                    for (unsigned int i = 0; i < dir.count; i++)
                    {
                        dir.entries[i].id = ((int *)ptr)[0];            // Resource id
                        dir.entries[i].offset = ((int *)ptr)[1];        // Resource offset in file
                        // NOTE: There is a reserved integer value before fileNameSize
                        dir.entries[i].fileNameSize = ((int *)ptr)[3];  // Resource fileName size

                        // Resource fileName, NULL terminated and 0-padded to 4-byte,
                        // fileNameSize considers NULL and padding
                        memcpy(dir.entries[i].fileName, ptr + 16, dir.entries[i].fileNameSize);

                        ptr += (16 + dir.entries[i].fileNameSize);      // Move pointer for next entry
                    }

                    RRES_FREE(chunkData.props);
                    RRES_FREE(chunkData.raw);
                }
            }
        }
        else RRES_LOG("RRES: WARNING: The provided file is not a valid rres file, file signature or version not valid\n");

        fclose(rresFile);
    }

    return dir;
}

// Unload central directory data
void rresUnloadCentralDirectory(rresCentralDir dir)
{
    RRES_FREE(dir.entries);
}

// Get rresResourceDataType from FourCC code
// NOTE: Function expects to receive a char[4] array
unsigned int rresGetDataType(const unsigned char *fourCC)
{
    unsigned int type = 0;

    if (fourCC != NULL)
    {
        if (memcmp(fourCC, "NULL", 4) == 0) type = RRES_DATA_NULL;              // Reserved for empty chunks, no props/data
        else if (memcmp(fourCC, "RAWD", 4) == 0) type = RRES_DATA_RAW;          // Raw file data, input file is not processed, just packed as is
        else if (memcmp(fourCC, "TEXT", 4) == 0) type = RRES_DATA_TEXT;         // Text file data, byte data extracted from text file
        else if (memcmp(fourCC, "IMGE", 4) == 0) type = RRES_DATA_IMAGE;        // Image file data, pixel data extracted from image file
        else if (memcmp(fourCC, "WAVE", 4) == 0) type = RRES_DATA_WAVE;         // Audio file data, samples data extracted from audio file
        else if (memcmp(fourCC, "VRTX", 4) == 0) type = RRES_DATA_VERTEX;       // Vertex file data, extracted from a mesh file
        else if (memcmp(fourCC, "FNTG", 4) == 0) type = RRES_DATA_FONT_GLYPHS;  // Font glyphs info, generated from an input font file
        else if (memcmp(fourCC, "LINK", 4) == 0) type = RRES_DATA_LINK;         // External linked file, filepath as provided on file input
        else if (memcmp(fourCC, "CDIR", 4) == 0) type = RRES_DATA_DIRECTORY;    // Central directory for input files relation to resource chunks
    }

    /*
    // Assign type (unsigned int) FourCC (char[4])
    if ((fourCC[0] == 'N') && (fourCC[1] == 'U') && (fourCC[2] == 'L') && (fourCC[3] == 'L')) type = RRES_DATA_NULL;             // NULL
    if ((fourCC[0] == 'R') && (fourCC[1] == 'A') && (fourCC[2] == 'W') && (fourCC[3] == 'D')) type = RRES_DATA_RAW;              // RAWD
    else if ((fourCC[0] == 'T') && (fourCC[1] == 'E') && (fourCC[2] == 'X') && (fourCC[3] == 'T')) type = RRES_DATA_TEXT;        // TEXT
    else if ((fourCC[0] == 'I') && (fourCC[1] == 'M') && (fourCC[2] == 'G') && (fourCC[3] == 'E')) type = RRES_DATA_IMAGE;       // IMGE
    else if ((fourCC[0] == 'W') && (fourCC[1] == 'A') && (fourCC[2] == 'V') && (fourCC[3] == 'E')) type = RRES_DATA_WAVE;        // WAVE
    else if ((fourCC[0] == 'V') && (fourCC[1] == 'R') && (fourCC[2] == 'T') && (fourCC[3] == 'X')) type = RRES_DATA_VERTEX;      // VRTX
    else if ((fourCC[0] == 'F') && (fourCC[1] == 'N') && (fourCC[2] == 'T') && (fourCC[3] == 'G')) type = RRES_DATA_FONT_GLYPHS; // FNTG
    else if ((fourCC[0] == 'L') && (fourCC[1] == 'I') && (fourCC[2] == 'N') && (fourCC[3] == 'K')) type = RRES_DATA_LINK;        // LINK
    else if ((fourCC[0] == 'C') && (fourCC[1] == 'D') && (fourCC[2] == 'I') && (fourCC[3] == 'R')) type = RRES_DATA_DIRECTORY;   // CDIR
    */

    return type;
}

// Get resource identifier from filename
// WARNING: It requires the central directory previously loaded
unsigned int rresGetResourceId(rresCentralDir dir, const char *fileName)
{
    unsigned int id = 0;

    for (unsigned int i = 0, len = 0; i < dir.count; i++)
    {
        len = (unsigned int)strlen(fileName);

        // NOTE: entries[i].fileName is NULL terminated and padded to 4-bytes
        if (strncmp((const char *)dir.entries[i].fileName, fileName, len) == 0)
        {
            id = dir.entries[i].id;
            break;
        }
    }

    return id;
}

// Compute CRC32 hash
// NOTE: CRC32 is used as rres id, generated from original filename
unsigned int rresComputeCRC32(const unsigned char *data, int len)
{
    static unsigned int crcTable[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
        0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
        0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
        0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
        0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
        0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
        0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
        0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
        0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
        0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
        0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
        0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
        0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
        0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
        0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
        0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
        0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
        0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    unsigned int crc = ~0u;

    for (int i = 0; i < len; i++) crc = (crc >> 8)^crcTable[data[i]^(crc&0xff)];

    return ~crc;
}

// Set password to be used on data decryption
void rresSetCipherPassword(const char *pass)
{
    password = pass;
}

// Get password to be used on data decryption
const char *rresGetCipherPassword(void)
{
    if (password == NULL) password = "password12345";

    return password;
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------
// Load user resource chunk from resource packed data (as contained in .rres file)
// WARNING: Data can be compressed and/or encrypted, in those cases is up to the user to process it,
// and chunk.data.propCount = 0, chunk.data.props = NULL and chunk.data.raw contains all resource packed data
static rresResourceChunkData rresLoadResourceChunkData(rresResourceChunkInfo info, void *data)
{
    rresResourceChunkData chunkData = { 0 };

    // CRC32 data validation, verify packed data is not corrupted
    unsigned int crc32 = rresComputeCRC32((const unsigned char *)data, info.packedSize);

    if ((rresGetDataType(info.type) != RRES_DATA_NULL) && (crc32 == info.crc32))   // Make sure chunk contains data and data is not corrupted
    {
        // Check if data chunk is compressed/encrypted to retrieve properties + data
        if ((info.compType == RRES_COMP_NONE) && (info.cipherType == RRES_CIPHER_NONE))
        {
            // Data is not compressed/encrypted (info.packedSize = info.baseSize)
            chunkData.propCount = ((unsigned int *)data)[0];

            if (chunkData.propCount > 0)
            {
                chunkData.props = (unsigned int *)RRES_CALLOC(chunkData.propCount, sizeof(unsigned int));
                for (unsigned int i = 0; i < chunkData.propCount; i++) chunkData.props[i] = ((unsigned int *)data)[i + 1];
            }

            int rawSize = info.baseSize - sizeof(int) - (chunkData.propCount*sizeof(int));
            chunkData.raw = RRES_CALLOC(rawSize, 1);
            if (chunkData.raw != NULL) memcpy(chunkData.raw, ((unsigned char *)data) + sizeof(int) + (chunkData.propCount*sizeof(int)), rawSize);
        }
        else
        {
            // Data is compressed/encrypted
            // We just return the loaded resource packed data from .rres file,
            // it's up to the user to manage decompression/decryption on user library
            chunkData.raw = RRES_CALLOC(info.packedSize, 1);
            if (chunkData.raw != NULL) memcpy(chunkData.raw, (unsigned char *)data, info.packedSize);
        }
    }

    if (crc32 != info.crc32) RRES_LOG("RRES: WARNING: [ID %i] CRC32 does not match, data can be corrupted\n", info.id);

    return chunkData;
}

#endif // RRES_IMPLEMENTATION
