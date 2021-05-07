/**********************************************************************************************
*
*   rres v1.0 - A simple and easy-to-use resource packaging file-format
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
*   rresFileHeader             (12 bytes)
*       File ID                 (4 bytes)   // File type id: 'rres'
*       Version                 (2 bytes)   // Format version
*       Res Count               (2 bytes)   // Number of resources contained
*       CD Offset               (4 bytes)   // Central Directory offset (if available)
*
*   rres resource chunk
*   {
*       rresInfoHeader         (32 bytes)
*           Type                (4 bytes)   // Resource type (FOURCC)
*           Id                  (4 bytes)   // Resource identifier (filename hash!)
*           Comp Type           (1 byte)    // Data compression type
*           Crypt Type          (1 byte)    // Data encryption type
*           Flags               (2 bytes)   // Data flags (if required)
*           Data CompSize       (4 bytes)   // Data compressed size
*           Data UncompSize     (4 bytes)   // Data uncompressed size
*           Next Offset         (4 bytes)   // Next resource offset (if required)
*           Reserved            (4 bytes)   // <reserved>
*           CRC32               (4 bytes)   // Data Chunk CRC32 (full chunk)
*   
*       rres Data Chunk         (n bytes)   // Data: [propsCount + props[n] + data]
*   }
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2016-2021 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef RRES_H
#define RRES_H

//#define RRES_STATIC
#if defined(RRES_STATIC)
    #define RRESDEF static              // Functions just visible to module including this file
#else
    #ifdef __cplusplus
        #define RRESDEF extern "C"      // Functions visible from other files (no name mangling of functions in C++)
    #else
        #define RRESDEF extern          // Functions visible from other files
    #endif
#endif

#define TRACELOG(level, ...) (void)0

// Check if custom malloc/free functions defined, if not, using standard ones
#if !defined(RRES_MALLOC)
    #include <stdlib.h>             // Required for: malloc(), free()

    #define RRES_MALLOC(size)       malloc(size)
    #define RRES_CALLOC(n,sz)       calloc(n,sz)
    #define RRES_FREE(ptr)          free(ptr)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define RRES_CURRENT_VERSION    100

//#define RRES_CDIR_MAXFN_LENGTH  512

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rres resource data chunk
// WARNING: This is the resource type returned to the user after
// reading and processing data from file, it's not aligned with
// internal resource data types: rresInfoHeader + dataChunk
typedef struct {
    unsigned int type;          // Resource chunk data type
    int propsCount;             // Resource chunk properties count
    int *props;                 // Resource chunk properties
    void *data;                 // Resource chunk data
} rresDataChunk;

// rres resource data
typedef struct {
    unsigned int count;         // Resource chunks count
    rresDataChunk *chunks;      // Resource chunks
} rresData;

// Specific data for some chunk types
//----------------------------------------------------------------------
// rres central directory entry
typedef struct {
    int id;                     // Resource unique id
    int offset;                 // Resource offset in file
    int fileNameLen;            // Resource fileName length
    char fileName[512];         // Resource fileName ('\0' terminated)
} rresDirEntry;

// rres central directory
typedef struct {
    int count;                  // Central directory entries count
    rresDirEntry *entries;      // Central directory entries 
} rresCentralDir;

// rres font glyphs info
typedef struct {
    int x, y, width, height;    // Glyph rectangle in the atals image
    int value;                  // Glyph codepoint value
    int offsetX, offsetY;       // Glyph drawing offset (from base line)
    int advanceX;               // Glyph advance X for next character
} rresFontGlyphsInfo;


// rres resource data type
// NOTE: Data type determines the proporties and the data included in every chunk
typedef enum {
    // Basic data (one chunk)
    //-----------------------------------------------------
    RRES_DATA_RAW       = 1,    // [RAWD] props[0]:size | data: raw bytes
    RRES_DATA_TEXT      = 2,    // [TEXT] props[0]:size, props[1]:cultureCode | data: text
    RRES_DATA_IMAGE     = 3,    // [IMGE] props[0]:width, props[1]:height, props[2]:mipmaps, props[3]:rresPixelFormat | data: pixels
    RRES_DATA_WAVE      = 4,    // [WAVE] props[0]:sampleCount, props[1]:sampleRate, props[2]:sampleSize, props[3]:channels | data: samples
    RRES_DATA_VERTEX    = 5,    // [VRTX] props[0]:vertexCount, props[1]:rresVertexAttribute, props[2]:rresVertexFormat | data: vertex
    RRES_DATA_FONT_INFO = 6,    // [FONT] props[0]:baseSize, props[1]:glyphsCount, props[2]:glyphsPadding | data: rresFontGlyphsInfo[0..glyphsCount]
    RRES_DATA_DIRECTORY = 100,  // [CDIR] props[0]:entryCount | data: rresDirEntry[0..entryCount]
    
    // Complex data (multiple chunks)
    //-----------------------------------------------------
    // Font consists of (2) chunks:
    //  - [FONT] rres[0]: RRES_DATA_FONT_INFO
    //  - [IMGE] rres[1]: RRES_DATA_IMAGE                    
    //
    // Mesh consists of (n) chunks:
    //  - [VRTX] rres[0]: RRES_DATA_VERTEX
    //  ...
    //  - [VRTX] rres[n]: RRES_DATA_VERTEX
    
    // NOTE: New rres data types can be added here with custom props|data
    
} rresDataType;

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

// Load all data chunks for specified rresId
RRESDEF rresData rresLoadData(const char *fileName, int rresId);
RRESDEF void rresUnloadData(rresData data);

RRESDEF rresCentralDir rresLoadCentralDirectory(const char *fileName);
RRESDEF void rresUnloadCentralDirectory(rresCentralDir dir);
RRESDEF int rresGetIdFromFileName(rresCentralDir dir, const char *fileName);

RRESDEF unsigned int rresComputeCRC32(unsigned char *buffer, int len);

#endif // RRES_H


/***********************************************************************************
*
*   RRES IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES_IMPLEMENTATION)

#include <stdio.h>              // Required for: FILE, fopen(), fclose()
#include <string.h>             // Required for: memcpy()

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rres file header (12 bytes)
typedef struct rresFileHeader {
    unsigned char id[4];        // File identifier: rRES
    unsigned short version;     // File version and subversion
    unsigned short chunkCount;  // Number of resource chunks in this file (total)
    unsigned int cdOffset;      // Central Directory offset in file (0 if not available)
} rresFileHeader;

// rres resource chunk info header (32 bytes)
typedef struct rresInfoHeader {
    unsigned char type[4];      // Resource chunk type (FOURCC)
    unsigned int id;            // Resource chunk identifier (filename hash!)
    unsigned char compType;     // Data compression type
    unsigned char crypType;     // Data encription type
    unsigned short flags;       // Data flags (if required)
    unsigned int compSize;      // Data compressed size
    unsigned int uncompSize;    // Data uncompressed size
    unsigned int nextOffset;    // Next resource chunk offset (if required)
    unsigned int reserved;
    unsigned int crc32;         // Data chunk CRC32 (propsCount + props[] + data)
} rresInfoHeader;

/*
// rres resource raw data chunk (as stored in file)
typedef struct rresFileDataChunk {
    rresInfoHeader info;        // Resource info header
    void *data;                 // Resource data chunk (props + data, compressed and encripted)
} rresFileDataChunk;
*/

//----------------------------------------------------------------------------------
// Enums Definition
//----------------------------------------------------------------------------------

// Compression types
typedef enum {
    RRES_COMP_NONE = 0,         // No data compression
    RRES_COMP_RLE,              // RLE compression
    RRES_COMP_DEFLATE,          // DEFLATE compression
    RRES_COMP_LZ4,              // LZ4 compression
    RRES_COMP_LZMA2,            // LZMA2 compression
    RRES_COMP_BZIP2,            // BZIP2 compression
    RRES_COMP_SNAPPY,           // SNAPPY compression
    RRES_COMP_BROTLI,           // BROTLI compression
    // gzip, zopfli, lzo, zstd  // Other compression algorythms...
} rresCompressionType;

// Encryption types
typedef enum {
    RRES_CRYPTO_NONE = 0,       // No data encryption
    RRES_CRYPTO_XOR,            // XOR (128 bit) encryption
    RRES_CRYPTO_AES,            // RIJNDAEL (128 bit) encryption (AES)
    RRES_CRYPTO_TDES,           // Triple DES encryption
    RRES_CRYPTO_BLOWFISH,       // BLOWFISH encryption
    RRES_CRYPTO_XTEA,           // XTEA encryption
    // twofish, RC5, RC6        // Other encryption algorythm...
} rresEncryptionType;

// Image/Texture data type
typedef enum {
    RRES_PIXELFORMAT_UNCOMP_GRAYSCALE = 1,      // 8 bit per pixel (no alpha)
    RRES_PIXELFORMAT_UNCOMP_GRAY_ALPHA,         // 16 bpp (2 channels)
    RRES_PIXELFORMAT_UNCOMP_R5G6B5,             // 16 bpp
    RRES_PIXELFORMAT_UNCOMP_R8G8B8,             // 24 bpp
    RRES_PIXELFORMAT_UNCOMP_R5G5B5A1,           // 16 bpp (1 bit alpha)
    RRES_PIXELFORMAT_UNCOMP_R4G4B4A4,           // 16 bpp (4 bit alpha)
    RRES_PIXELFORMAT_UNCOMP_R8G8B8A8,           // 32 bpp
    RRES_PIXELFORMAT_UNCOMPRESSED_R32,          // 32 bpp (1 channel - float)
    RRES_PIXELFORMAT_UNCOMPRESSED_R32G32B32,    // 32*3 bpp (3 channels - float)
    RRES_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, // 32*4 bpp (4 channels - float)
    RRES_PIXELFORMAT_COMP_DXT1_RGB,             // 4 bpp (no alpha)
    RRES_PIXELFORMAT_COMP_DXT1_RGBA,            // 4 bpp (1 bit alpha)
    RRES_PIXELFORMAT_COMP_DXT3_RGBA,            // 8 bpp
    RRES_PIXELFORMAT_COMP_DXT5_RGBA,            // 8 bpp
    RRES_PIXELFORMAT_COMP_ETC1_RGB,             // 4 bpp
    RRES_PIXELFORMAT_COMP_ETC2_RGB,             // 4 bpp
    RRES_PIXELFORMAT_COMP_ETC2_EAC_RGBA,        // 8 bpp
    RRES_PIXELFORMAT_COMP_PVRT_RGB,             // 4 bpp
    RRES_PIXELFORMAT_COMP_PVRT_RGBA,            // 4 bpp
    RRES_PIXELFORMAT_COMP_ASTC_4x4_RGBA,        // 8 bpp
    RRES_PIXELFORMAT_COMP_ASTC_8x8_RGBA         // 2 bpp
    //...
} rresPixelFormat;

// Vertex data attribute
typedef enum {
    RRES_VERTEX_ATTRIBUTE_POSITION,
    RRES_VERTEX_ATTRIBUTE_TEXCOORD1,
    RRES_VERTEX_ATTRIBUTE_TEXCOORD2,
    RRES_VERTEX_ATTRIBUTE_TEXCOORD3,
    RRES_VERTEX_ATTRIBUTE_TEXCOORD4,
    RRES_VERTEX_ATTRIBUTE_NORMAL,
    RRES_VERTEX_ATTRIBUTE_TANGENT,
    RRES_VERTEX_ATTRIBUTE_COLOR,
    RRES_VERTEX_ATTRIBUTE_INDEX,
    //...
} rresVertexAttribute;

// Vertex data format type
typedef enum {
    RRES_VERTEX_FORMAT_BYTE,
    RRES_VERTEX_FORMAT_SHORT,
    RRES_VERTEX_FORMAT_INT,
    RRES_VERTEX_FORMAT_HFLOAT,
    RRES_VERTEX_FORMAT_FLOAT,
    //...
} rresVertexFormat;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static rresDataChunk rresLoadDataChunk(rresInfoHeader info, void *data);
static void rresUnloadDataChunk(rresDataChunk chunk);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load resource from file by id
// NOTE: All resources conected to base id are loaded
rresData rresLoadData(const char *fileName, int rresId)
{
    rresData rres = { 0 };

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) TRACELOG(LOG_WARNING, "[%s] rres file could not be opened", fileName);
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
                rresInfoHeader info = { 0 };
                
                // Read resource info header
                fread(&info, sizeof(rresInfoHeader), 1, rresFile);

                if (info.id == rresId)
                {
                    rres.count = 1;
                    
                    int currentResPosition = SEEK_CUR;      // Store current file position
                    rresInfoHeader temp = info;             // Temp info header to scan resource chunks
                    
                    // Scan all resource chunks checking temp.nextOffset
                    while (temp.nextOffset != 0)
                    {
                        fseek(rresFile, temp.nextOffset, SEEK_SET);             // Jump to next resource
                        fread(&temp, sizeof(rresInfoHeader), 1, rresFile);      // Read next resource info header
                        rres.count++;
                    }
                        
                    rres.chunks = (rresDataChunk *)RRES_CALLOC(rres.count, sizeof(rresDataChunk));   // Load as many rres slots as required
                    fseek(rresFile, currentResPosition, SEEK_SET);              // Return to first resource chunk position
                    
                    if (rres.count > 1) TRACELOG(LOG_INFO, "[%s][ID %i] Multiple resource chunks detected: %i chunks", fileName, (int)info.id, *count);

                    int i = 0;
                    
                    // Load all required resource chunks
                    while (info.nextOffset != 0)
                    {
                        void *data = RRES_MALLOC(info.compSize);
                        fread(data, info.compSize, 1, rresFile);
                        
                        // Load chunk data from raw file data
                        // NOTE: Raw data can be compressed and encrypted,
                        // it can also contain data properties at the beginning
                        rres.chunks[i] = rresLoadDataChunk(info, data);
                        RRES_FREE(data);
                        
                        fseek(rresFile, temp.nextOffset, SEEK_SET);             // Jump to next resource
                        fread(&info, sizeof(rresInfoHeader), 1, rresFile);      // Read next resource info header
                        
                        i++;
                    }                  

                    break;      // Resource id found, stop checking the file
                }
                else
                {
                    // Skip required data size to read next resource infoHeader
                    fseek(rresFile, info.compSize, SEEK_CUR);
                }
            }
        }

        fclose(rresFile);
    }

    return rres;
}

// Unload resource data
void rresUnloadData(rresData rres)
{
    for (int i = 0; i < rres.count; i++) rresUnloadDataChunk(rres.chunks[i]);
    
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
            TraceLog(LOG_WARNING, "CDIR: Found at offset: %08x", header.cdOffset);
            
            rresInfoHeader info = { 0 };
            
            fseek(rresFile, header.cdOffset, SEEK_CUR);         // Move to central directory position
            fread(&info, sizeof(rresInfoHeader), 1, rresFile);  // Read resource info
           
            if ((info.type[0] == 'C') &&
                (info.type[1] == 'D') &&
                (info.type[2] == 'I') &&
                (info.type[3] == 'R'))
            {
                TraceLog(LOG_WARNING, "CDIR: Found! Comp.Size: %i", info.compSize);
                
                void *data = RRES_MALLOC(info.compSize);
                fread(data, info.compSize, 1, rresFile);
                
                rresDataChunk chunk = rresLoadDataChunk(info, data);
                //RRES_FREE(data);
                
                //dir.count = chunk.props[0];                 // Files count
                
                //dir.entries = (rresDirEntry *)chunk.data;   // Files data
                /*
                char *ptr = &chunk.data;
                dir.entries = (rresDirEntry *)RRES_CALLOC(dir.count, sizeof(rresDirEntry));
                
                for (int i = 0; i < dir.count; i++)
                {
                    dir.entries[i].id = (int *)ptr;         // Resource unique id
                    ptr += 4;
                    dir.entries[i].offset = (int *)ptr;     // Resource offset in file
                    ptr += 4;
                    dir.entries[i].fileNameLen = (int *)ptr;    // Resource fileName length
                    ptr += 4;
                    memcpy(dir.entries[i].fileName, ptr, dir.entries[i].fileNameLen);   // Resource fileName ('\0' terminated)
                    ptr += dir.entries[i].fileNameLen + 1;
                }
                */
                //rresUnloadDataChunk(chunk);
            }
        }

        fclose(rresFile);
    }
    
    return dir;
}

void rresUnloadCentralDirectory(rresCentralDir dir)
{
    for (int i = 0; i < dir.count; i++) RRES_FREE(dir.entries[i].fileName);

    RRES_FREE(dir.entries);
}

int rresGetIdFromFileName(rresCentralDir dir, const char *fileName)
{
    int id = 0;
    
    for (int i = 0; i < dir.count; i++)
    {
        if (strcmp((const char *)dir.entries[i].fileName, fileName) == 0) 
        { 
            id = dir.entries[i].id;
            break; 
        }
    }

    return id;
}

// Compute CRC32
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
// Module specific Functions Definition
//----------------------------------------------------------------------------------
// Load data chunk from raw resource data as contained in file
// NOTE: Raw data can be compressed and encrypted, it also contains the props data embedded
static rresDataChunk rresLoadDataChunk(rresInfoHeader info, void *data)
{
    rresDataChunk chunk = { 0 };
    void *result = NULL;

    // Assign rres.type (int) from info.type (FOURCC)
    if ((info.type[0] == 'R') && (info.type[0] == 'A') && (info.type[0] == 'W') && (info.type[0] == 'D')) chunk.type = 1;        // RAWD
    else if ((info.type[0] == 'T') && (info.type[0] == 'E') && (info.type[0] == 'X') && (info.type[0] == 'T')) chunk.type = 2;   // TEXT
    else if ((info.type[0] == 'I') && (info.type[0] == 'M') && (info.type[0] == 'G') && (info.type[0] == 'E')) chunk.type = 3;   // IMGE
    else if ((info.type[0] == 'W') && (info.type[0] == 'A') && (info.type[0] == 'V') && (info.type[0] == 'E')) chunk.type = 4;   // WAVE
    else if ((info.type[0] == 'V') && (info.type[0] == 'R') && (info.type[0] == 'T') && (info.type[0] == 'X')) chunk.type = 5;   // VRTX
    else if ((info.type[0] == 'F') && (info.type[0] == 'O') && (info.type[0] == 'N') && (info.type[0] == 'T')) chunk.type = 10;  // FONT
    else if ((info.type[0] == 'C') && (info.type[0] == 'D') && (info.type[0] == 'I') && (info.type[0] == 'R')) chunk.type = 100; // CDIR

    // Decompress and decrypt [properties + data] chunk
    // TODO: Support multiple compression/encryption types
    if (info.compType == RRES_COMP_NONE)
    {
        result = RRES_MALLOC(info.compSize);
        memcpy(result, data, info.compSize);
    }
    else if (info.compType == RRES_COMP_DEFLATE)
    {
        //result = Decompress(data, info.compSize, info.uncompSize);   // TODO.
    }

    // CRC32 data validation
    int crc32 = rresComputeCRC32(result, info.uncompSize);
    
    if (crc32 == info.crc32)    // Check CRC32
    {
        chunk.propsCount = ((int *)result)[0];
        
        if (chunk.propsCount > 0)
        {
            chunk.props = (int *)RRES_MALLOC(chunk.propsCount*sizeof(int));
            for (int i = 0; i < chunk.propsCount; i++) chunk.props[i] = ((int *)result)[1 + i];
        }
    
        chunk.data = RRES_MALLOC(info.uncompSize);
        memcpy(chunk.data, ((unsigned char *)result) + (chunk.propsCount*sizeof(int)), info.uncompSize);
    }
    else 
    {
        TRACELOG(LOG_WARNING, "[ID %i] CRC32 does not match, data can be corrupted", info.id);
        if (info.compType == RRES_COMP_DEFLATE) RRES_FREE(result);
    }

    return chunk;
}

// Unload data chunk
static void rresUnloadDataChunk(rresDataChunk chunk)
{
    RRES_FREE(chunk.props);   // Resource chunk properties
    RRES_FREE(chunk.data);    // Resource chunk data
}

#endif // RRES_IMPLEMENTATION
