/**********************************************************************************************
*
*   rres v0.9 - raylib resource custom fileformat management functions
*
*   CONFIGURATION:
*
*   #define RREM_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   DEPENDENCIES:
*       tinfl   -  DEFLATE decompression functions
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2016-2018 Ramon Santamaria (@raysan5)
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

/*  
References:
    RIFF file-format:  http://www.johnloomis.org/cpe102/asgn/asgn1/riff.html
    ZIP file-format:   https://en.wikipedia.org/wiki/Zip_(file_format)
                       http://www.onicos.com/staff/iz/formats/zip.html
    XNB file-format:   http://xbox.create.msdn.com/en-US/sample/xnb_format
*/

#ifndef RRES_H
#define RRES_H

#if !defined(RRES_STANDALONE)
    #include "raylib.h"
#endif

//#define RRES_STATIC
#ifdef RRES_STATIC
    #define RRESDEF static              // Functions just visible to module including this file
#else
    #ifdef __cplusplus
        #define RRESDEF extern "C"      // Functions visible from other files (no name mangling of functions in C++)
    #else
        #define RRESDEF extern          // Functions visible from other files
    #endif
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define MAX_RESOURCES_SUPPORTED   256

//----------------------------------------------------------------------------------
// Types and Structures Definition
// NOTE: Some types are required for RAYGUI_STANDALONE usage
//----------------------------------------------------------------------------------
#if defined(RRES_STANDALONE)
    // rRES resource chunk (piece of data)
    typedef struct RRESChunk {
        unsigned int type;          // Resource chunk type (4 byte)
        int *params;                // Resource chunk parameters pointer (4 byte)
        void *data;                 // Resource chunk data pointer (4 byte)
    } RRESChunk;
    
    // rRES resource (single resource)
    // NOTE: One resource could be composed of multiple chunks
    typedef struct RRES {
        unsigned int type;          // Resource type (4 bytes)
        unsigned int resCount;      // Resource chunks counter
        RRESChunk *chunks;          // Resource chunks
    } RRES;
    
    // rRES resources array
    // NOTE: It could be useful at some point to load multiple resources
    typedef struct RRES *RRESList;  // Resources list pointer
    
    // RRESData type
    typedef enum { 
        RRES_TYPE_RAW = 0,          // Basic raw type, no parameters
        RRES_TYPE_IMAGE,            // Basic image type, [4] parameters: width, height, mipmaps, format
        RRES_TYPE_WAVE,             // Basic wave type, [4] parameters: sampleCount, sampleRate, sampleSize, channels
        RRES_TYPE_MESH,             // Complex mesh type, multiple RRES_TYPE_VERTEX chunks
        RRES_TYPE_MATERIAL,         // Complex material type, multiple RRES_TYPE_IMAGE chunks, custom parameters
        RRES_TYPE_VERTEX,           // Basic vertex type, [4] parameters: vertexCount, vertexType, vertexFormat
        RRES_TYPE_TEXT,             // Basic text type, [2] parameters: charsCount, cultureCode
        RRES_TYPE_SPRITEFONT,       // Complex font type, RRES_TYPE_IMAGE + RRES_TYPE_FONT_CHARDATA chunks
        RRES_TYPE_FONT_CHARDATA,    // Basic chars-info type, [2] parameters: charsCount, baseSize
        RRES_TYPE_DIRECTORY         // Basic directory type, [2] parameters: fileCount, directoryCount
    } RRESDataType;
#endif

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
//RRESDEF RRESData LoadResourceData(const char *rresFileName, int rresId, int part);
RRESDEF RRES LoadRRES(const char *fileName, unsigned int rresId);
RRESDEF void UnloadRRES(RRES rres);

/*
QUESTION: How to load each type of data from RRES ?

rres->type == RRES_TYPE_RAW
unsigned char data = (unsigned char *)rres[0]->data;

rres->type == RRES_TYPE_IMAGE
Image image;
image.data = rres[0]->data;        // Be careful, duplicate pointer
image.width = rres[0]->param1;
image.height = rres[0]->param2;
image.mipmaps = rres[0]->param3;
image.format =  rres[0]->format;

rres->type == RRES_TYPE_WAVE
Wave wave;
wave.data = rres[0]->data;
wave.sampleCount = rres[0]->param1;
wave.sampleRate = rres[0]->param2;
wave.sampleSize = rres[0]->param3;
wave.channels = rres[0]->param4;

rres->type == RRES_TYPE_VERTEX (multiple parts)
Mesh mesh;
mesh.vertexCount = rres[0]->param1;
mesh.vertices = (float *)rres[0]->data;
mesh.texcoords = (float *)rres[1]->data;
mesh.normals = (float *)rres[2]->data;
mesh.tangents = (float *)rres[3]->data;
mesh.tangents = (unsigned char *)rres[4]->data;

rres->type == RRES_TYPE_TEXT
unsigned char *text = (unsigned char *)rres->data;
Shader shader = LoadShaderText(text, rres->param1);     Shader LoadShaderText(const char *shdrText, int length);

rres->type == RRES_TYPE_FONT_IMAGE      (multiple parts)
rres->type == RRES_TYPE_FONT_CHARDATA   
SpriteFont font;
font.texture = LoadTextureFromImage(image);     // rres[0]
font.chars = (CharInfo *)rres[1]->data;
font.charsCount = rres[1]->param1;
font.baseSize = rres[1]->param2;

rres->type == RRES_TYPE_DIRECTORY
unsigned char *fileNames = (unsigned char *)rres[0]->data;  // fileNames separed by \n
int filesCount = rres[0]->param1;
*/

#endif // RRES_H


/***********************************************************************************
*
*   RRES IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES_IMPLEMENTATION)

#include <stdio.h>          // Required for: FILE, fopen(), fclose()

// Check if custom malloc/free functions defined, if not, using standard ones
#if !defined(RRES_MALLOC)
    #include <stdlib.h>     // Required for: malloc(), free()

    #define RRES_MALLOC(size)  malloc(size)
    #define RRES_FREE(ptr)     free(ptr)
#endif

#include "external/tinfl.c" // Required for: tinfl_decompress_mem_to_mem()
                            // NOTE: DEFLATE algorythm data decompression

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// rRES file header (8 byte)
typedef struct {
    unsigned char id[4];            // File identifier: rRES (4 byte)
    unsigned short version;         // File version and subversion (2 byte)
    unsigned short count;           // Number of resources in this file (2 byte)
} RRESFileHeader;

// rRES resource info header (12 bytes)
typedef struct {
    unsigned int id;                // Resource unique identifier (4 byte)
    unsigned short resType;         // Resource type (simple/multiple chunks)
    unsigned short chunkCount;      // Resource chunks counter
    unsigned int resTotalSize;      // Resource total size (for all chunks)
} RRESInfoHeader;

// rRES resource chunk info header (16 bytes + 4 bytes*paramsCount)
typedef struct {
    unsigned char chunkType;        // Resource chunk type (1 byte)
    unsigned char compType;         // Resource chunk compression type (1 byte)
    unsigned char crypType;         // Resource chunk encryption type (1 byte)
    unsigned char paramsCount;      // Resource chunk parameters count (1 byte)
    unsigned int crc32;             // Resource chunk CRC32 for data (4 byte)
    unsigned int dataSize;          // Resource chunk size (compressed or not, only DATA) (4 byte)
    unsigned int uncompSize;        // Resource chunk size (uncompressed, only DATA) (4 byte)

    int *params;                    // Resouce chunk parameters pointer (4 byte*paramsCount)
} RRESChunkInfoHeader;

// rRES resource chunk (info + data)
// NOTE: Useful for resource chunk creation API
typedef struct {
    RRESChunkInfoHeader chunkInfo;  // Resource chunk info header
    void *data;                     // Resource chunk data
} RRESResourceChunk;

// rRES resource (complete)
// NOTE: Useful for resource creation API
typedef struct {
    RRESInfoHeader infoHeader;      // Resource info header
    RRESResourceChunk *chunks;      // Resource chunks
} RRESResource;

//----------------------------------------------------------------------------------
// Enums Definition
//----------------------------------------------------------------------------------

// Compression types
typedef enum { 
    RRES_COMP_NONE = 0,         // No data compression
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
    RRES_VERT_BYTE,
    RRES_VERT_SHORT,
    RRES_VERT_INT,
    RRES_VERT_HFLOAT,
    RRES_VERT_FLOAT,
    //...
} RRESVertexFormat;

#if defined(RRES_STANDALONE)
typedef enum { 
    LOG_INFO = 0, 
    LOG_ERROR, 
    LOG_WARNING, 
    LOG_DEBUG, 
    LOG_OTHER 
} TraceLogType;
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static void *DecompressData(const unsigned char *data, unsigned long compSize, int uncompSize);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load resource from file by id (could contain multiple chunks)
// NOTE: Returns uncompressed data with parameters, searches resource by id
RRESDEF RRES LoadRRES(const char *fileName, unsigned int rresId)
{
    RRES rres = { 0 };
    
    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) TraceLog(LOG_WARNING, "[%s] rRES raylib resource file could not be opened", fileName);
    else
    {
        RRESFileHeader fileHeader;
    
        // Read rres file info header
        fread(&fileHeader.id[0], sizeof(char), 1, rresFile);
        fread(&fileHeader.id[1], sizeof(char), 1, rresFile);
        fread(&fileHeader.id[2], sizeof(char), 1, rresFile);
        fread(&fileHeader.id[3], sizeof(char), 1, rresFile);
        fread(&fileHeader.version, sizeof(short), 1, rresFile);
        fread(&fileHeader.count, sizeof(short), 1, rresFile);

        // Verify "rRES" identifier
        if ((fileHeader.id[0] != 'r') && (fileHeader.id[1] != 'R') && (fileHeader.id[2] != 'E') && (fileHeader.id[3] != 'S'))
        {
            TraceLog(LOG_WARNING, "[%s] This is not a valid raylib resource file", fileName);
        }
        else
        {  
            for (int i = 0; i < fileHeader.count; i++)
            {
                RRESInfoHeader infoHeader;
                
                // Read resource info and parameters
                fread(&infoHeader, sizeof(RRESInfoHeader), 1, rresFile);
                
                if (infoHeader.id == rresId)
                {
                    rres.type = infoHeader.resType;
                    rres.chunkCount = infoHeader.chunkCount;
                    rres.chunks = (RRESChunk *)RRES_MALLOC(sizeof(RRESChunk)*infoHeader.chunkCount);

                    // Load all required resource chunks
                    for (int k = 0; k < infoHeader.chunkCount; k++)
                    {
                        RRESChunkInfoHeader chunkInfoHeader;
                
                        // Read resource chunk info and parameters
                        fread(&chunkInfoHeader, sizeof(RRESChunkInfoHeader), 1, rresFile);
                
                        // Register data type and read parameters
                        rres.chunks[k].type = chunkInfoHeader.chunkType;
                        rres.chunks[k].params = (int *)RRES_MALLOC(sizeof(int)*chunkInfoHeader.paramsCount);
                        
                        for (int p = 0; p < chunkInfoHeader.paramsCount; p++) fread(&rres.chunks[k].params[p], sizeof(int), 1, rresFile);
 
                        // Read resource chunk data block
                        void *data = RRES_MALLOC(chunkInfoHeader.dataSize);
                        fread(data, chunkInfoHeader.dataSize, 1, rresFile);
                        
                        // TODO: Check CRC32

                        // Decompres and decrypt data if required
                        // TODO: Support multiple decompression/decryption algorythms
                        if (chunkInfoHeader.compType == RRES_COMP_DEFLATE)
                        {
                            // NOTE: Memory is allocated inside Decompress data, should be freed manually
                            void *uncompData = DecompressData(data, chunkInfoHeader.dataSize, chunkInfoHeader.uncompSize);
                            
                            rres.chunks[k].data = uncompData;
                            
                            RRES_FREE(data);
                        }
                        else rres.chunks[k].data = data;

                        if (rres.chunks[k].params != NULL) TraceLog(LOG_INFO, "[%s][ID %i] Resource parameters loaded successfully", fileName, (int)infoHeader.id);
                        if (rres.chunks[k].data != NULL) TraceLog(LOG_INFO, "[%s][ID %i] Resource data loaded successfully", fileName, (int)infoHeader.id);
                    }
                }
                else
                {
                    // Skip required data to read next resource infoHeader
                    fseek(rresFile, infoHeader.resTotalSize, SEEK_CUR);
                }
            }
            
            if (rres.chunks[0].data == NULL) TraceLog(LOG_WARNING, "[%s][ID %i] Requested resource could not be found", fileName, (int)rresId);
        }

        fclose(rresFile);
    }

    return rres;
}

// Unload resource data
RRESDEF void UnloadRRES(RRES rres)
{
    for (int i = 0; i < rres.chunkCount; i++)
    {
        if (rres.chunks[i].params != NULL) free(rres.chunks[i].params);
        if (rres.chunks[i].data != NULL) free(rres.chunks[i].data);
    }
}

// Init rRES resource file (write file header)
RRESDEF void InitRRES(const char *fileName)
{
    FILE *rresFile = fopen(fileName, "rb");

    if (rresFile == NULL) TraceLog(LOG_WARNING, "[%s] rRES raylib resource file could not be opened", fileName);
    else
    {
        RRESFileHeader fileHeader;
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
    // TODO: Append rRES resource to file
}

// Save additional resource chunk of data
// NOTE: RRESResource info header is modified
RRESDEF void AppendRRESChunk(RRESResource *resource, RRESResourceChunk chunk)
{
    // TODO: Append resource chunk to existing resource
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------

// Data decompression function
// NOTE: Allocated data MUST be freed by user
static void *DecompressData(const unsigned char *data, unsigned long compSize, int uncompSize)
{
    int tempUncompSize;
    void *uncompData;

    // Allocate buffer to hold decompressed data
    uncompData = (mz_uint8 *)RRES_MALLOC((size_t)uncompSize);

    // Check correct memory allocation
    if (uncompData == NULL)
    {
        TraceLog(LOG_WARNING, "Out of memory while decompressing data");
    }
    else
    {
        // Decompress data
        tempUncompSize = tinfl_decompress_mem_to_mem(uncompData, (size_t)uncompSize, data, compSize, 1);

        if (tempUncompSize == -1)
        {
            TraceLog(LOG_WARNING, "Data decompression failed");
            RRES_FREE(uncompData);
        }

        if (uncompSize != (int)tempUncompSize)
        {
            TraceLog(LOG_WARNING, "Expected uncompressed size do not match, data may be corrupted");
            TraceLog(LOG_WARNING, " -- Expected uncompressed size: %i", uncompSize);
            TraceLog(LOG_WARNING, " -- Returned uncompressed size: %i", tempUncompSize);
        }

        TraceLog(LOG_INFO, "Data decompressed successfully from %u bytes to %u bytes", (mz_uint32)compSize, (mz_uint32)tempUncompSize);
    }

    return uncompData;
}

// Some required functions for rres standalone module version
#if defined(RRES_STANDALONE)
// Show trace log messages (LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG)
void TraceLog(int logType, const char *text, ...)
{
    va_list args;
    va_start(args, text);

    switch (msgType)
    {
        case LOG_INFO: fprintf(stdout, "INFO: "); break;
        case LOG_ERROR: fprintf(stdout, "ERROR: "); break;
        case LOG_WARNING: fprintf(stdout, "WARNING: "); break;
        case LOG_DEBUG: fprintf(stdout, "DEBUG: "); break;
        default: break;
    }

    vfprintf(stdout, text, args);
    fprintf(stdout, "\n");

    va_end(args);

    if (msgType == LOG_ERROR) exit(1);
}
#endif

#endif // RRES_IMPLEMENTATION
