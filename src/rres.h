/**********************************************************************************************
*
*   rres v1.0 - raylib resource custom fileformat management functions
*
*   CONFIGURATION:
*
*   #define RREM_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define SUPPORT_LIBRARY_MINIZ
*       miniz library is included and used for DEFLATE compression/decompression
*
*   RRES FILE STRUCTURE:
*
*   RRES File Header            (12 bytes)  [RRESFileHeader]
*       File ID                 (4 bytes)   // File type id: 'rRES'
*       Version                 (2 bytes)   // Format version
*       Res Count               (2 bytes)   // Number of resources contained
*       CD Offset               (4 bytes)   // Central Directory offset (if available)
*
*   RRES Resource [RRES]
*   {
*       RRES Info Header        (32 bytes)  [RRESInfoHeader]
*           Type                (4 bytes)   // Resource type (FOURCC)
*           Id                  (4 bytes)   // Resource unique identifier
*           Comp Type           (1 byte)    // DATA compression type
*           Crypt Type          (1 byte)    // DATA encryption type
*           Flags               (2 bytes)   // Flags (if required)
*           Data CompSize       (4 bytes)   // DATA compressed size
*           Data UncompSize     (4 bytes)   // DATA uncompressed size
*           Next Offset         (4 bytes)   // Offset to next resource related
*           Reserved            (4 bytes)   // <reserved>
*           CRC32               (4 bytes)   // Data Chunk CRC32 (full chunk)
*   
*       RRES Data Chunk         (n bytes)   // DATA: [props + data]
*   }
*
*   DEPENDENCIES:
*       tinfl   - DEFLATE decompression library
*       miniz   - DEFLATE compression/decompression library (zlib-style)
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2016-2020 Ramon Santamaria (@raysan5)
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

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define RRES_CURRENT_VERSION    100

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rRES resource chunk (piece of data)
// WARNING: No aligned with internal RRES structure
typedef struct RRESData {
    unsigned int type;          // Resource data type
    int propsCount;             // Resource properties count
    int *props;                 // Resource properties
    void *data;                 // Resource data
} RRESData;

// RRES Central Directory Entry
typedef struct {
    int id;                     // Resource unique ID
    int offset;                 // Resource offset in file
    int fileNameLen;            // Resource fileName length
    char *fileName;             // Resource fileName ('\0' terminated)
} RRESDirEntry;

// RRES Central Directory
typedef struct {
    int count;                  // CDir entries count
    RRESDirEntry *entries;      // CDir entries 
} RRESCentralDir;

// RRESData type
typedef enum {
    RRES_TYPE_RAWFILE   = 1,    // [RAWD] Basic type: no properties / Data: raw
    RRES_TYPE_TEXT      = 2,    // [TEXT] Basic type: [2] properties: charsCount, cultureCode / Data: text
    RRES_TYPE_IMAGE     = 3,    // [IMGE] Basic type: [4] properties: width, height, mipmaps, format / Data: pixels
    RRES_TYPE_WAVE      = 4,    // [WAVE] Basic type: [4] properties: sampleCount, sampleRate, sampleSize, channels / Data: samples
    RRES_TYPE_VERTEX    = 5,    // [VRTX] Basic type: [4] properties: vertexCount, vertexType, vertexFormat / Data: vertex
    RRES_TYPE_FONT      = 10,   // [FONT] Complex type:
                                //          RRES0: [n] properties: charsCount, baseSize, recs / Data: -
                                //          RRES1: RRES_TYPE_IMAGE
                                //          RRES2: RRES_TYPE_CHARDATA [optional]
    RRES_TYPE_CHARDATA  = 11,   // [CHAR] Complex type: 
                                //          RRES0: [1] property: charsCount / Data: -
                                //          {
                                //              RRESn: [4] properties: value, offsetX, offsetY, advanceX / Data: -
                                //              RRESn+1: RRES_TYPE_IMAGE
                                //          }
    RRES_TYPE_MESH      = 12,   // [MESH] Complex type: 
                                //          RRES0: [1] property: vertexBuffersCount / Data: -
                                //          {
                                //              RRESn: RRES_TYPE_VERTEX
                                //          }
    RRES_TYPE_MATERIAL  = 13,   // [MATD] Complex type: 
                                //          RRES0: [n] properties: mapsCount, params / Data: -
                                //          {
                                //              RRESn: [2] properties: color, value_comp / Data: -
                                //              RRESn+1: RRES_TYPE_IMAGE
                                //          }
    RRES_TYPE_MODEL     = 20,   // [MODL] Complex type:
                                //          RRES0: [n] properties: meshCount, materialCount, transform, meshMaterial / Data: -
                                //          {
                                //              RRESn: RRES_TYPE_MESH
                                //              RRESm: RRES_TYPE_MATDATA
                                //          }
    RRES_TYPE_DIRECTORY = 100,  // [CDIR] Basic type: [1] properties: fileCount / Data: RRESDirEntry[]
} RRESDataType;

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

// Load all data chunks under provided rresId,
RRESDEF RRESData LoadRRES(const char *fileName, int rresId);
RRESDEF RRESData *LoadRRESMulti(const char *fileName, int rresId, int *count);
RRESDEF void UnloadRRES(RRESData rres);

RRESDEF RRESCentralDir LoadRRESCentralDirectory(const char *fileName);
RRESDEF int GetRRESIdFromFileName(RRESCentralDir dir, const char *fileName);

#endif // RRES_H


/***********************************************************************************
*
*   RRES IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES_IMPLEMENTATION)

#include <stdio.h>              // Required for: FILE, fopen(), fclose()

// Check if custom malloc/free functions defined, if not, using standard ones
#if !defined(RRES_MALLOC)
    #include <stdlib.h>         // Required for: malloc(), free()

    #define RRES_MALLOC(size)   malloc(size)
    #define RRES_CALLOC(n,sz)   calloc(n,sz)
    #define RRES_FREE(ptr)      free(ptr)
#endif

#define SUPPORT_LIBRARY_MINIZ

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rRES file header (12 bytes)
typedef struct RRESFileHeader {
    unsigned char id[4];        // File identifier: rRES
    unsigned short version;     // File version and subversion
    unsigned short resCount;    // Number of resources in this file (total)
    unsigned int cdOffset;      // Central Directory offset in file (0 if not available)
} RRESFileHeader;

// rRES resource info header (32 bytes)
typedef struct RRESInfoHeader {
    unsigned char type[4];      // Resource type (FOURCC)
    unsigned int id;            // Resource unique identifier
    unsigned char compType;     // Data compression type
    unsigned char crypType;     // Data encription type
    unsigned short flags;       // Data flags (if required)
    unsigned int compSize;      // Data compressed size (if compressed)
    unsigned int uncompSize;    // Data uncompressed size
    unsigned int nextOffset;    // Next resource offset (if required)
    unsigned int reserved;      // <reserved>
    unsigned int crc32;         // Data chunk CRC32 (full chunk)
} RRESInfoHeader;

// rRES resource chunk
typedef struct {
    RRESInfoHeader info;        // Resource info header
    void *dataChunk;            // Resource data chunk (data unprocessed)
} RRES;

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
} RRESCompressionType;

// Encryption types
typedef enum {
    RRES_CRYPTO_NONE = 0,       // No data encryption
    RRES_CRYPTO_XOR,            // XOR (128 bit) encryption
    RRES_CRYPTO_AES,            // RIJNDAEL (128 bit) encryption (AES)
    RRES_CRYPTO_TDES,           // Triple DES encryption
    RRES_CRYPTO_BLOWFISH,       // BLOWFISH encryption
    RRES_CRYPTO_XTEA,           // XTEA encryption
    // twofish, RC5, RC6        // Other encryption algorythm...
} RRESEncryptionType;

// Image/Texture data type
typedef enum {
    RRES_IM_UNCOMP_GRAYSCALE = 1,     // 8 bit per pixel (no alpha)
    RRES_IM_UNCOMP_GRAY_ALPHA,        // 16 bpp (2 channels)
    RRES_IM_UNCOMP_R5G6B5,            // 16 bpp
    RRES_IM_UNCOMP_R8G8B8,            // 24 bpp
    RRES_IM_UNCOMP_R5G5B5A1,          // 16 bpp (1 bit alpha)
    RRES_IM_UNCOMP_R4G4B4A4,          // 16 bpp (4 bit alpha)
    RRES_IM_UNCOMP_R8G8B8A8,          // 32 bpp
    RRES_IM_COMP_DXT1_RGB,            // 4 bpp (no alpha)
    RRES_IM_COMP_DXT1_RGBA,           // 4 bpp (1 bit alpha)
    RRES_IM_COMP_DXT3_RGBA,           // 8 bpp
    RRES_IM_COMP_DXT5_RGBA,           // 8 bpp
    RRES_IM_COMP_ETC1_RGB,            // 4 bpp
    RRES_IM_COMP_ETC2_RGB,            // 4 bpp
    RRES_IM_COMP_ETC2_EAC_RGBA,       // 8 bpp
    RRES_IM_COMP_PVRT_RGB,            // 4 bpp
    RRES_IM_COMP_PVRT_RGBA,           // 4 bpp
    RRES_IM_COMP_ASTC_4x4_RGBA,       // 8 bpp
    RRES_IM_COMP_ASTC_8x8_RGBA        // 2 bpp
    //...
} RRESImageFormat;

// Vertex data type
typedef enum {
    RRES_VERT_POSITION,
    RRES_VERT_TEXCOORD1,
    RRES_VERT_TEXCOORD2,
    RRES_VERT_TEXCOORD3,
    RRES_VERT_TEXCOORD4,
    RRES_VERT_NORMAL,
    RRES_VERT_TANGENT,
    RRES_VERT_COLOR,
    RRES_VERT_INDEX,
    //...
} RRESVertexType;

// Vertex data format type
typedef enum {
    RRES_VERT_FORMAT_BYTE,
    RRES_VERT_FORMAT_SHORT,
    RRES_VERT_FORMAT_INT,
    RRES_VERT_FORMAT_HFLOAT,
    RRES_VERT_FORMAT_FLOAT,
    //...
} RRESVertexFormat;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
#if defined(SUPPORT_LIBRARY_MINIZ)
static unsigned char *CompressDEFLATE(const unsigned char *data, unsigned long uncompSize, unsigned long *outCompSize);
static unsigned char *DecompressDEFLATE(const unsigned char *data, unsigned long compSize, int uncompSize);
#endif

static RRESData GetDataFromChunk(RRESInfoHeader info, void *dataChunk);
static unsigned int ComputeCRC32(unsigned char *buffer, int len);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load single resource from file by id
// NOTE: Only first resource with id found is loaded
RRESData LoadRRES(const char *fileName, int rresId)
{
    RRESData rres = { 0 };
    int resCount = 0;

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) TRACELOG(LOG_WARNING, "[%s] rRES file could not be opened", fileName);
    else
    {
        RRESFileHeader header = { 0 };

        // Read rres file header
        fread(&header, sizeof(RRESFileHeader), 1, rresFile);

        // Verify "rRES" identifier
        if ((header.id[0] == 'r') && 
            (header.id[1] == 'R') && 
            (header.id[2] == 'E') && 
            (header.id[3] == 'S'))
        {
            for (int i = 0; i < header.resCount; i++)
            {
                RRESInfoHeader info = { 0 };
                
                // Read resource info header
                fread(&info, sizeof(RRESInfoHeader), 1, rresFile);

                if (info.id == rresId)
                {
                    rres.type = 0;//info.type;  // TODO.
                    
                    void *dataChunk = RRES_MALLOC(info.compSize);
                    fread(dataChunk, info.compSize, 1, rresFile);
                    
                    // Decompres and decrypt data if required
                    rres = GetDataFromChunk(info, dataChunk);
                    
                    RRES_FREE(dataChunk);
                    
                    if (info.nextOffset != 0) TRACELOG(LOG_WARNING, "[%s][ID %i] Multiple related resources detected!", fileName, (int)info.id);

                    break;
                }
                else
                {
                    // Skip required data to read next resource infoHeader
                    fseek(rresFile, info.compSize, SEEK_CUR);
                }
            }

            if ((rres.propsCount == 0) && (rres.data == NULL)) TRACELOG(LOG_WARNING, "[%s][ID %i] Requested resource could not be found", fileName, rresId);
        }

        fclose(rresFile);
    }

    return rres;
}

// Load multiple related resource from file by id
// NOTE: All related resources to base id are loaded
RRESData *LoadRRESMulti(const char *fileName, int rresId, int *count)
{
    RRESData *rres = NULL;
    *count = 0;

    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) TRACELOG(LOG_WARNING, "[%s] rRES file could not be opened", fileName);
    else
    {
        RRESFileHeader header = { 0 };

        // Read rres file header
        fread(&header, sizeof(RRESFileHeader), 1, rresFile);

        // Verify "rRES" identifier
        if ((header.id[0] == 'r') && 
            (header.id[1] == 'R') && 
            (header.id[2] == 'E') && 
            (header.id[3] == 'S'))
        {
            for (int i = 0; i < header.resCount; i++)
            {
                RRESInfoHeader info = { 0 };
                
                // Read resource info header
                fread(&info, sizeof(RRESInfoHeader), 1, rresFile);

                if (info.id == rresId)
                {
                    *count = 1;
                    
                    int currentResPosition = SEEK_CUR;      // Store current file position
                    RRESInfoHeader temp = info;             // Temp info header to scan related resources
                    
                    // Scan all related resources checking temp.nextOffset
                    while (temp.nextOffset != 0)
                    {
                        fseek(rresFile, temp.nextOffset, SEEK_SET);             // Jump to next resource
                        fread(&temp, sizeof(RRESInfoHeader), 1, rresFile);      // Read next resource info header
                        *count++;
                    }
                        
                    rres = (RRESData *)RRES_MALLOC(*count*sizeof(RRESData));    // Load as many rres slots as required
                    fseek(rresFile, currentResPosition, SEEK_SET);              // Return to first resource position
                    
                    if (*count > 1) TRACELOG(LOG_INFO, "[%s][ID %i] Multiple related resources detected: %i resources", fileName, (int)info.id, *count);

                    int i = 0;
                    
                    // Load all required related resources
                    while (info.nextOffset != 0)
                    {
                        void *dataChunk = RRES_MALLOC(info.compSize);
                        fread(dataChunk, info.compSize, 1, rresFile);
                        
                        // Decompres and decrypt data if required
                        rres[i] = GetDataFromChunk(info, dataChunk);
                        RRES_FREE(dataChunk);
                        
                        if ((rres[i].propsCount == 0) && (rres[i].data == NULL)) TRACELOG(LOG_WARNING, "[%s][ID %i] Requested resource could not be found", fileName, rresId);
                        
                        fseek(rresFile, temp.nextOffset, SEEK_SET);             // Jump to next resource
                        fread(&info, sizeof(RRESInfoHeader), 1, rresFile);      // Read next resource info header
                        
                        i++;
                    }                  

                    break;
                }
                else
                {
                    // Skip required data to read next resource infoHeader
                    fseek(rresFile, info.compSize, SEEK_CUR);
                }
            }
        }

        fclose(rresFile);
    }

    return rres;
}

// Unload resource data
void UnloadRRES(RRESData rres)
{
    free(rres.props);
    free(rres.data);
}

// Load central directory data
RRESCentralDir LoadCentralDirectory(const char *fileName)
{
    RRESCentralDir dir = { 0 };
    
    FILE *rresFile = fopen(fileName, "rb");
    
    if (rresFile != NULL)
    {
        RRESFileHeader header = { 0 };
        
        fread(&header, sizeof(RRESFileHeader), 1, rresFile);
        
        if ((header.id[0] == 'r') && 
            (header.id[1] == 'R') && 
            (header.id[2] == 'E') && 
            (header.id[3] == 'S') &&
            (header.cdOffset != 0))
        {
            RRESInfoHeader info = { 0 };
            
            fseek(rresFile, header.cdOffset, SEEK_CUR);         // Move to central directory position
            fread(&info, sizeof(RRESInfoHeader), 1, rresFile);  // Read resource info
           
            if ((info.type[0] == 'C') &&
                (info.type[1] == 'D') &&
                (info.type[2] == 'I') &&
                (info.type[3] == 'R'))
            {
                void *dataChunk = RRES_MALLOC(info.compSize);
                fread(dataChunk, info.compSize, 1, rresFile);
                
                RRESData rres = GetDataFromChunk(info, dataChunk);
                RRES_FREE(dataChunk);
                
                dir.count = rres.props[0];                      // Files count
                dir.entries = (RRESDirEntry *)rres.data;        // Files data
            }
        }

        fclose(rresFile);
    }
    
    return dir;
}

int GetIdFromFileName(RRESCentralDir dir, const char *fileName)
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

RRESData GetDataFromChunk(RRESInfoHeader info, void *dataChunk)
{
    RRESData rres = { 0 };
    void *result = NULL;

    // Assign rres.type (int) from info.type (FOURCC)
    if (strncmp(info.type, "RAWD", 4)) rres.type = 1;
    else if (strncmp(info.type, "TEXT", 4)) rres.type = 2;
    else if (strncmp(info.type, "IMGE", 4)) rres.type = 3;
    else if (strncmp(info.type, "WAVE", 4)) rres.type = 4;
    else if (strncmp(info.type, "VRTX", 4)) rres.type = 5;
    else if (strncmp(info.type, "FONT", 4)) rres.type = 10;
    else if (strncmp(info.type, "CHAR", 4)) rres.type = 11;
    else if (strncmp(info.type, "MESH", 4)) rres.type = 12;
    else if (strncmp(info.type, "MATD", 4)) rres.type = 13;
    else if (strncmp(info.type, "MODL", 4)) rres.type = 20;
    else if (strncmp(info.type, "CDIR", 4)) rres.type = 100;

    // Decompress and decrypt [properties + data] chunk
    // TODO: Support multiple compression/encryption types
    if (info.compType == RRES_COMP_NONE) result = dataChunk;
    else if (info.compType == RRES_COMP_DEFLATE) result = DecompressDEFLATE(dataChunk, info.compSize, info.uncompSize);

    int crc32 = ComputeCRC32(result, info.uncompSize);
    
    if (crc32 == info.crc32)    // Check CRC32
    {
        rres.propsCount = ((int *)result)[0];
        
        if (rres.propsCount > 0)
        {
            rres.props = (int *)RRES_MALLOC(rres.propsCount*sizeof(int));
            for (int i = 0; i < rres.propsCount; i++) rres.props[i] = ((int *)result)[1 + i];
        }
    
        rres.data = RRES_MALLOC(info.uncompSize);
        memcpy(rres.data, result + (rres.propsCount*sizeof(int)), info.uncompSize);
    }
    else 
    {
        TRACELOG(LOG_WARNING, "[ID %i] CRC32 does not match, data can be corrupted", info.id);
        
        if (info.compType == RRES_COMP_DEFLATE) RRES_FREE(result);
    }

    return rres;
}

/*
// Init rRES resource file (write file header)
RRESDEF void InitRRES(const char *fileName)
{
    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) TRACELOG(LOG_WARNING, "[%s] rRES raylib resource file could not be opened", fileName);
    else
    {
        RRESFileHeader header = { 0 };
        fileHeader.id[0] = 'r';
        fileHeader.id[1] = 'R';
        fileHeader.id[2] = 'E';
        fileHeader.id[3] = 'S';
        fileHeader.version = 100;
        fileHeader.count = 1;

        // Write rres file header into file
        fwrite(&fileHeader, sizeof(RRESFileHeader), 1, rresFile);
    }

    fclose(rresFile);
}

// Add additional resource to existing rres file
RRESDEF void AppendRRESResource(const char *fileName, RRESResource resource)
{
    // TODO: Append resource to file
}
*/

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------

/*
// Data decompression (using DEFLATE, tinfl library)
// NOTE: Allocated data MUST be freed by user
static unsigned char *DecompressDEFLATE(const unsigned char *data, unsigned long compSize, int uncompSize)
{
    int tempUncompSize;
    void *uncompData;

    // Allocate buffer to hold decompressed data
    uncompData = (mz_uint8 *)RRES_MALLOC((size_t)uncompSize);

    // Check correct memory allocation
    if (uncompData == NULL)
    {
        TRACELOG(LOG_WARNING, "Out of memory while decompressing data");
    }
    else
    {
        // Decompress data
        tempUncompSize = tinfl_decompress_mem_to_mem(uncompData, (size_t)uncompSize, data, compSize, 1);

        if (tempUncompSize == -1)
        {
            TRACELOG(LOG_WARNING, "Data decompression failed");
            RRES_FREE(uncompData);
        }

        if (uncompSize != tempUncompSize)
        {
            TRACELOG(LOG_WARNING, "Expected uncompressed size do not match, data may be corrupted");
            TRACELOG(LOG_WARNING, " -- Expected uncompressed size: %i", uncompSize);
            TRACELOG(LOG_WARNING, " -- Returned uncompressed size: %i", tempUncompSize);
        }

        TRACELOG(LOG_INFO, "Data decompressed successfully from %u bytes to %u bytes", (mz_uint32)compSize, (mz_uint32)tempUncompSize);
    }

    return uncompData;
}
*/

#if defined(SUPPORT_LIBRARY_MINIZ)
// Data compression (using DEFLATE, miniz library)
// NOTE: Allocated data MUST be freed!
static unsigned char *CompressDEFLATE(const unsigned char *data, unsigned long uncompSize, unsigned long *outCompSize)
{
    int compStatus;
    unsigned long tempCompSize = compressBound(uncompSize);
    unsigned char *pComp;

    // Allocate buffer to hold compressed data
    pComp = (mz_uint8 *)malloc((size_t)tempCompSize);

    TRACELOG(LOG_INFO, "Compression space reserved: %i\n", tempCompSize);

    // Check correct memory allocation
    if (!pComp)
    {
        TRACELOG(LOG_INFO, "Out of memory!\n");
        return NULL;
    }

    // Compress data
    compStatus = compress(pComp, &tempCompSize, (const unsigned char *)data, uncompSize);

    // Check compression success
    if (compStatus != Z_OK)
    {
        TRACELOG(LOG_INFO, "Compression failed!\n");
        free(pComp);
        return NULL;
    }

    TRACELOG(LOG_INFO, "Compressed from %u bytes to %u bytes\n", (mz_uint32)uncompSize, (mz_uint32)tempCompSize);

    if (tempCompSize > uncompSize) TRACELOG(LOG_INFO, "WARNING: Compressed data is larger than uncompressed data!!!\n");

    *outCompSize = tempCompSize;

    return pComp;
}

// Data decompression (using DEFLATE, miniz library)
// NOTE: Allocated data MUST be freed!
static unsigned char *DecompressDEFLATE(const unsigned char *data, unsigned long compSize, int uncompSize)
{
    int decompStatus;
    unsigned long tempUncompSize;
    unsigned char *pUncomp;

    // Allocate buffer to hold decompressed data
    pUncomp = (mz_uint8 *)malloc((size_t)uncompSize);

    // Check correct memory allocation
    if (!pUncomp)
    {
        TRACELOG(LOG_INFO, "Out of memory!\n");
        return NULL;
    }

    // Decompress data
    decompStatus = uncompress(pUncomp, &tempUncompSize, data, compSize);

    if (decompStatus != Z_OK)
    {
        TRACELOG(LOG_INFO, "Decompression failed!\n");
        free(pUncomp);
        return NULL;
    }

    if (uncompSize != (int)tempUncompSize)
    {
        TRACELOG(LOG_INFO, "WARNING! Expected uncompressed size do not match! Data may be corrupted!\n");
        TRACELOG(LOG_INFO, " -- Expected uncompressed size: %i\n", uncompSize);
        TRACELOG(LOG_INFO, " -- Returned uncompressed size: %i\n", tempUncompSize);
    }

    TRACELOG(LOG_INFO, "Decompressed from %u bytes to %u bytes\n", (mz_uint32)compSize, (mz_uint32)tempUncompSize);

    return pUncomp;
}
#endif

// Data compression data (custom RLE algorythm)
static unsigned char *CompressDataRLE(const unsigned char *data, unsigned int uncompSize, unsigned int *outCompSize)
{
    unsigned char *compData = (unsigned char *)malloc(uncompSize * 2 * sizeof(unsigned char));    // NOTE: We allocate some initial space to store compresed data,
    // hopefully, it will be < uncompSize but in the worst possible case it could be 2 * uncompSize, so we allocate that space just in case...

    TRACELOG(LOG_INFO, "Compresed data array allocated! Size: %i\n", uncompSize * 2);

    unsigned char count = 1;
    unsigned char currentValue, nextValue;

    int j = 0;

    currentValue = data[0];

    TRACELOG(LOG_INFO, "First initial value: %i\n", currentValue);
    //getchar();

    for (int i = 1; i < uncompSize; i++)
    {
        nextValue = data[i];

        if (currentValue == nextValue)
        {
            if (count == 255)
            {
                compData[j] = count;
                compData[j + 1] = currentValue;

                j += 2;
                count = 1;
            }
            else count++;
        }
        else
        {
            compData[j] = count;
            compData[j + 1] = currentValue;

            //TRACELOG(LOG_INFO, "Data stored Value-Count: %i - %i\n", currentValue, count);

            j += 2;
            count = 1;

            currentValue = nextValue;
        }
    }

    compData[j] = count;
    compData[j + 1] = currentValue;
    j += 2;

    TRACELOG(LOG_INFO, "Data stored Value-Count: %i - %i\n", currentValue, count);

    compData[j] = 0;    // Just to indicate the end of data
                        // NOTE: Array lenght will be j

    TRACELOG(LOG_INFO, "Data compressed!\n");

    // Resize memory block with realloc
    compData = (unsigned char *)realloc(compData, j * sizeof(unsigned char));

    if (compData == NULL)
    {
        TRACELOG(LOG_INFO, "Error reallocating memory!");

        free(compData);    // Free the initial memory block
    }

    // REMEMBER: compData must be freed!

    //unsigned char *outData = (unsigned char *)malloc((j+1) * sizeof(unsigned char));

    //for (int i = 0; i < (j+1); i++) outData[i] = compData[i];

    *outCompSize = (j + 1);  // New array of compressed data lenght

    return compData;    // REMEMBER! This memory should be freed!
}

// Compute CRC32
static unsigned int ComputeCRC32(unsigned char *buffer, int len)
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

#endif // RRES_IMPLEMENTATION
