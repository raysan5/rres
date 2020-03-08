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

/*
*   rRES Resource Data                  // Depending on resource type, some 'properties' preceed data
*       Image Data Params   (6 bytes)
*           Width           (2 byte)    // Image width
*           Height          (2 byte)    // Image height
*           Color format    (1 byte)    // Image data color format
*                                           // 0 - Monocrome (1 bit per pixel)
*                                           // 1 - Grayscale (8 bit)
*                                           // 2 - R5G6B5 (16 bit)
*                                           // 3 - R5G5B5A1 (16 bit)
*                                           // 4 - R4G4B4A4 (16 bit)
*                                           // 5 - R8G8B8 (24 bit)
*                                           // 6 - R8G8B8A8 (32 bit) - default
*           Mipmaps count   (1 byte)    // Mipmap images included - default 1
*           DATA                        // Image data
*       Sound Data Params   (6 bytes)
*           Sample Rate     (2 byte)    // Sample rate (frequency)
*           BitsPerSample   (2 byte)    // Bits per sample
*           Channels        (1 byte)    // Channels (1 - mono, 2 - stereo)
*           <reserved>      (1 byte)
*           DATA                        // Sound data
*       Vertex Data Params  (6 bytes?)
*           Num vertex      (4 byte)    // Number of vertex
*           Data Type       (1 byte)    // Vertex arrays provided --> Use like FLAGS: 01011101
*                                           // 0001 - Positions
*                                           // 0011 - position+textcoord
*                                           // position-color-UVcoords-normals-otherUV
*           Indexed Data    // Indexing data will be GREAT!...but it must be calculated!
*                           // Once all data is unindexed, just test positions-UVcoords-normals and create new index!
*                           // Or just store unindexed data... easier!
*           DATA                        // Model vertex data (32 bit per vertex - 1 float)
*       Text Data Params
*           DATA                        // Text data (processed like raw data), '\0' ended
*       Raw Data Params <no-params>
*           DATA                        // Raw data
*/

/*
References:
    FOURCC:            https://en.wikipedia.org/wiki/FourCC
    RIFF file-format:  http://www.johnloomis.org/cpe102/asgn/asgn1/riff.html
    ZIP file-format:   https://en.wikipedia.org/wiki/Zip_(file_format)
                       http://www.onicos.com/staff/iz/formats/zip.html
    XNB file-format:   http://xbox.create.msdn.com/en-US/sample/xnb_format
    
    RIFF:   Type | len | format | <-- DATA -->
    ZIP:    SIGN | VER | FLAGS | CompType | ModTime | ModDate | CRC32 | CompSize | UncompSize | nameLen | fileName | <--- DATA --->
    PNG:    Length | ChunkType | <-- DATA --> | CRC32
    
    GLB:    Length | ChunkType | <-- DATA -->

*/

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

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define RRES_MAX_RESOURCES      256
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
    RRES_TYPE_RAW = 0,          // [RAWD] Basic type: no properties
    RRES_TYPE_TEXT,             // [TEXT] Basic type: [2] properties: charsCount, cultureCode
    RRES_TYPE_IMAGE,            // [IMGE] Basic type: [4] properties: width, height, mipmaps, format
    RRES_TYPE_WAVE,             // [WAVE] Basic type: [4] properties: sampleCount, sampleRate, sampleSize, channels
    RRES_TYPE_VERTEX,           // [VRTX] Basic type: [4] properties: vertexCount, vertexType, vertexFormat
    RRES_TYPE_MATDATA,          // [MATD] Basic type: [n] properties-values
    RRES_TYPE_FONT,             // [FONT] Complex type: [n] properties: charsCount, baseSize, recs -> [1] RRES_TYPE_IMAGE + RRES_TYPE_CHARDATA chunks
    RRES_TYPE_CHARDATA,         // [CHAR] Basic type: no properties
    RRES_TYPE_MESH,             // [MESH] Complex type: [n] RRES_TYPE_VERTEX chunks
    RRES_TYPE_MODEL,            // [MODL] Complex type: [n] RRES_TYPE_MESH + [n] RRES_TYPE_MATERIAL chunks
    RRES_TYPE_DIRECTORY         // [CDIR] Basic type, [1] properties: fileCount
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

/*
QUESTION: How to load each type of data from RRES -> Custom implementations (library dependant)

// RRES_TYPE_RAW: char *data = (char *)rres.data;

Image LoadImageFromRRES(RRESData rres)
{
    Image image = { 0 };
    
    if ((rres.count == 1) && (rres.type == RRES_TYPE_IMAGE))
    {
        image.width = rres.chunk[0].properties[0];
        image.height = rres.chunk[0].properties[1];
        image.mipmaps = rres.chunk[0].properties[2];
        image.format =  rres.chunk[0].properties[3];
        
        image.data = rres.chunk[0].data;     // WARNING: Pointer assignment! -> Shallow copy?
    }
    
    return image;
}

Wave LoadWaveFromRRES(RRESData rres)
{
    Wave wave = { 0 };
    
    if ((rres.count == 1) && (rres.type == RRES_TYPE_WAVE))
    {
        wave.sampleCount = rres.chunk[0].properties[0];
        wave.sampleRate = rres.chunk[0].properties[1];
        wave.sampleSize = rres.chunk[0].properties[2];
        wave.channels = rres.chunk[0].properties[3];
        
        wave.data = rres.chunk[0].data;     // WARNING: Pointer assignment! -> Shallow copy?
    }
}

Font LoadFontFromRRES(RRESData rres)
{
    Font font = { 0 };

    if ((rres.count == 2) && (rres.type == RRES_TYPE_FONT))

    // To load a Font we expect at least 2 resource data packages
    if (rres[0].type == RRES_TYPE_FONT_IMAGE)
    {
        Image image = LoadImageFromRRES(rres[0]);
        font.texture = LoadTextureFromImage(image);
    }
    
    if (rres[1].type == RRES_TYPE_FONT_CHARDATA) //-> RAW DATA?
    {
        font.charsCount = rres[1].properties[0];
        font.baseSize = rres[1].properties[1];
        
        // Font rectangle properties come as individual properties
        font.recs = (Rectangle *)RRES_MALLOC(font.charsCount*sizeof(Rectangle));
        
        for (int i = 0; i < font.charsCount; i++)
        {
            font.recs[i].x = (float)rres[1].properties[2 + i*4];
            font.recs[i].y = (float)rres[1].properties[2 + i*4 + 1];
            font.recs[i].width = (float)rres[1].properties[2 + i*4 + 2];
            font.recs[i].height = (float)rres[1].properties[2 + i*4 + 3];
        }
        
        font.chars = (CharInfo *)rres[1]->data;
    }

    return font;
}

rres->type == RRES_TYPE_VERTEX (multiple parts)
Mesh mesh;
mesh.vertexCount = rres[0]->param1;
mesh.vertices = (float *)rres[0]->data;
mesh.texcoords = (float *)rres[1]->data;
mesh.normals = (float *)rres[2]->data;
mesh.tangents = (float *)rres[3]->data;
mesh.tangents = (unsigned char *)rres[4]->data;

rres->type == RRES_TYPE_TEXT
unsigned char *text = (unsigned char *)rres->data;  // Text must be '\0' terminated -> Save it this way!
Shader shader = LoadShaderText(text, rres->param1);     Shader LoadShaderText(const char *shdrText, int length);
*/
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
    #define RRES_FREE(ptr)      free(ptr)
#endif

#include "external/tinfl.c"     // Required for: tinfl_decompress_mem_to_mem()
                                // NOTE: DEFLATE algorythm data decompression

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
static void *DecompressData(const unsigned char *data, unsigned long compSize, int uncompSize);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load resource from file by id (could contain multiple chunks)
// NOTE: Returns uncompressed data with properties, searches resource by id
RRESDEF RRESData LoadRRES(const char *fileName, int rresId)
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
            for (int i = 0; i < header.count; i++)
            {
                RRESInfoHeader info = { 0 };
                
                // Read resource info header
                fread(&info, sizeof(RRESInfoHeader), 1, rresFile);

                if (info.id == rresId)
                {
                    //if (info.nextOffset != 0) TRACELOG(LOG_INFO, "Multiple resources related detected");
                    
                    // TODO: Scan all linked resources checking rres.info.nextOffset
                    //rres = (RRESData *)RRES_MALLOC(resCount*sizeof(RRESData));
                    
                    // Load all required resource chunks
                    while (info.nextOffset != 0)
                    {
                        void *dataChunk = RRES_MALLOC(info.compSize);
                        fread(dataChunk, info.compSize, 1, rresFile);
                        
                        // Decompres and decrypt data if required
                        data[resCount] = GetDataFromChunk(rres.info, dataChunk);
                        
                        //if (data[resCount].props != NULL) TRACELOG(LOG_INFO, [%s][ID %i] Resource properties loaded successfully", fileName, (int)info.id);
                        //if (data[resCount].data != NULL) TRACELOG(LOG_INFO, "[%s][ID %i] Resource data loaded successfully", fileName, (int)info.id);
                        
                        free(dataChunk);
                        
                        fseek(rresFile, info.nextOffset + sizeof(RRESFileHeader), SEEK_SET);
                        
                        // Read next resource info header
                        fread(&info, sizeof(RRESInfoHeader), 1, rresFile);
                        
                        resCount++;
                    }                  

                    //if (rres.chunks[k].properties != NULL) TRACELOG(LOG_INFO, "[%s][ID %i] Resource properties loaded successfully", fileName, (int)infoHeader.id);
                    //if (rres.chunks[k].data != NULL) TRACELOG(LOG_INFO, "[%s][ID %i] Resource data loaded successfully", fileName, (int)infoHeader.id);
                
                    break;
                }
                else
                {
                    // Skip required data to read next resource infoHeader
                    fseek(rresFile, rres.info.compSize, SEEK_CUR);
                }
            }

            if (rres.dataChunk == NULL) TRACELOG(LOG_WARNING, "[%s][ID %i] Requested resource could not be found", fileName, rresId);
        }

        fclose(rresFile);
    }

    return data;
}

// Unload resource data
RRESDEF void UnloadRRES(RRESData rres)
{
    free(rres.chunk.properties);
    free(rres.chunk.data);
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
        
        if ((header.id[0] == 'r') && (header.id[1] == 'R') && (header.id[2] == 'E') && (header.id[3] == 'S'))
        {
            if (header.cdOffset != 0)
            {
                // Move to central directory position
                fseek(rresFile, header.cdOffset, SEEK_SET);
                
                RRES rres = { 0 };
                
                // Read central directory info
                fread(&rres.info, sizeof(RRESInfoHeader), 1, rresFile);
                fread(rres.data, rres.info.compData, 1, rresFile);
                
                // Read chunk data for usage (uncompress, unencrypt, check CRC32...)
                RRESData rresData = GetRRESDataFromChunk(rres.info, rres.chunk);
                
                if (rresData.type == RRES_TYPE_DIRECTORY)
                {
                    dir.count = rresData.properties[0];   // Files count
                    dir.entries = (RRESDirEntry *)rresData.data;
                }
            }
        }

        fclose(fileName);
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
            id = dir.entries[i].rresId; 
            break; 
        }
    }

    return id;
}

RRESData GetDataFromChunk(RRESInfoHeader info, void *dataChunk)
{
    RRESData rres = { 0 };
    
    rres.type = info.type;   // TODO: Assign rres.type (int) from info.type (FOURCC)

    // TODO: Check CRC32
    
    // TODO: Decompress and decrypt [properties + data] chunk
    
    if (info.compType == RRES_COMP_DEFLATE)
    {
        void *result = DecompressData(dataChunk, info.compSize, info.uncompSize);
        
        rres.propsCount = ((int *)result)[0];
        
        if (propsCount > 0)
        {
            rres.props = (int *)RRES_MALLOC(rres.propsCount*sizeof(int));
            for (int i = 0; i < rres.propsCount; i++) rres.props[i] = ((int *)result)[1 + i];
        }
    
        rres.data = RRES_MALLOC(info.uncompSize);
        memcpy(rres.data, result + (rres.propsCount*sizeof(int)), info.uncompSize);
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

// Data decompression (using DEFLATE, tinfl library)
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

/*
// Data compression (using DEFLATE, miniz library)
// NOTE: Allocated data MUST be freed!
static unsigned char *CompressData(const unsigned char *data, unsigned long uncompSize, unsigned long *outCompSize)
{
    int compStatus;
    unsigned long tempCompSize = compressBound(uncompSize);
    unsigned char *pComp;

    // Allocate buffer to hold compressed data
    pComp = (mz_uint8 *)malloc((size_t)tempCompSize);

    printf("Compression space reserved: %i\n", tempCompSize);

    // Check correct memory allocation
    if (!pComp)
    {
        printf("Out of memory!\n");
        return NULL;
    }

    // Compress data
    compStatus = compress(pComp, &tempCompSize, (const unsigned char *)data, uncompSize);

    // Check compression success
    if (compStatus != Z_OK)
    {
        printf("Compression failed!\n");
        free(pComp);
        return NULL;
    }

    printf("Compressed from %u bytes to %u bytes\n", (mz_uint32)uncompSize, (mz_uint32)tempCompSize);

    if (tempCompSize > uncompSize) printf("WARNING: Compressed data is larger than uncompressed data!!!\n");

    *outCompSize = tempCompSize;

    return pComp;
}

// Data decompression (using DEFLATE, miniz library)
// NOTE: Allocated data MUST be freed!
static unsigned char *DecompressData(const unsigned char *data, unsigned long compSize, int uncompSize)
{
    int decompStatus;
    unsigned long tempUncompSize;
    unsigned char *pUncomp;

    // Allocate buffer to hold decompressed data
    pUncomp = (mz_uint8 *)malloc((size_t)uncompSize);

    // Check correct memory allocation
    if (!pUncomp)
    {
        printf("Out of memory!\n");
        return NULL;
    }

    // Decompress data
    decompStatus = uncompress(pUncomp, &tempUncompSize, data, compSize);

    if (decompStatus != Z_OK)
    {
        printf("Decompression failed!\n");
        free(pUncomp);
        return NULL;
    }

    if (uncompSize != (int)tempUncompSize)
    {
        printf("WARNING! Expected uncompressed size do not match! Data may be corrupted!\n");
        printf(" -- Expected uncompressed size: %i\n", uncompSize);
        printf(" -- Returned uncompressed size: %i\n", tempUncompSize);
    }

    printf("Decompressed from %u bytes to %u bytes\n", (mz_uint32)compSize, (mz_uint32)tempUncompSize);

    return pUncomp;
}

// Data compression data (custom RLE algorythm)
static unsigned char *CompressDataRLE(const unsigned char *data, unsigned int uncompSize, unsigned int *outCompSize)
{
    unsigned char *compData = (unsigned char *)malloc(uncompSize * 2 * sizeof(unsigned char));    // NOTE: We allocate some initial space to store compresed data,
    // hopefully, it will be < uncompSize but in the worst possible case it could be 2 * uncompSize, so we allocate that space just in case...

    printf("Compresed data array allocated! Size: %i\n", uncompSize * 2);

    unsigned char count = 1;
    unsigned char currentValue, nextValue;

    int j = 0;

    currentValue = data[0];

    printf("First initial value: %i\n", currentValue);
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

            //printf("Data stored Value-Count: %i - %i\n", currentValue, count);

            j += 2;
            count = 1;

            currentValue = nextValue;
        }
    }

    compData[j] = count;
    compData[j + 1] = currentValue;
    j += 2;

    printf("Data stored Value-Count: %i - %i\n", currentValue, count);

    compData[j] = 0;    // Just to indicate the end of data
                        // NOTE: Array lenght will be j

    printf("Data compressed!\n");

    // Resize memory block with realloc
    compData = (unsigned char *)realloc(compData, j * sizeof(unsigned char));

    if (compData == NULL)
    {
        printf("Error reallocating memory!");

        free(compData);    // Free the initial memory block
    }

    // REMEMBER: compData must be freed!

    //unsigned char *outData = (unsigned char *)malloc((j+1) * sizeof(unsigned char));

    //for (int i = 0; i < (j+1); i++) outData[i] = compData[i];

    *outCompSize = (j + 1);  // New array of compressed data lenght

    return compData;    // REMEMBER! This memory should be freed!
}
*/

#endif // RRES_IMPLEMENTATION
