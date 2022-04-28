/**********************************************************************************************
*
*   rres-raylib v1.0 - rres loaders specific for raylib data structures
*
*   CONFIGURATION:
*
*   #define RRES_RAYLIB_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define RRES_SUPPORT_COMPRESSION_LZ4
*       Support data compression LZ4 algorithm, provided by lz4.h/lz4.c library
* 
*   #define RRES_SUPPORT_ENCRYPTION_MONOCYPHER
*       Support data encryption algorithms provided by monocypher.h/monocypher.c library
* 
* 
*   LICENSE: MIT
*
*   Copyright (c) 2020-2022 Ramon Santamaria (@raysan5)
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

#ifndef RRES_RAYLIB_H
#define RRES_RAYLIB_H

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

// rres data loading to raylib data structures
RRESAPI void *rresLoadRaw(rresResource rres, int *size);    // Load raw data from rres resource
RRESAPI char *rresLoadText(rresResource rres);              // Load text data from rres resource
RRESAPI Image rresLoadImage(rresResource rres);             // Load Image data from rres resource
RRESAPI Wave rresLoadWave(rresResource rres);               // Load Wave data from rres resource
RRESAPI Font rresLoadFont(rresResource rres);               // Load Font data from rres resource
RRESAPI Mesh rresLoadMesh(rresResource rres);               // Load Mesh data from rres resource
//RRESAPI Material rresLoadMaterial(rresResource rres);       // TODO: Load Material data from rres resource
//RRESAPI Model rresLoadModel(rresResource rres);             // TODO: Load Model data from rres resource

// Utilities for encrypted data
// NOTE: The cipher password is kept as the pointer provided, 
// no internal copy is done, it's up to the user to manage that sensitive data properly
// It's recommended to allocate the password previously to rres file loading and wipe that memory space after rres loading
RRESAPI void rresSetCipherPassword(const char *pass);       // Set password to be used in case of encrypted data found

#endif // RRES_RAYLIB_H

/***********************************************************************************
*
*   RRES RAYLIB IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES_RAYLIB_IMPLEMENTATION)

// Include supported compression/encryption algorithms
// NOTE: They should be the same supported by the rres packaging tool (rrespacker)
#if defined(RRES_SUPPORT_COMPRESSION_LZ4)
    #include "external/lz4.h"               // LZ4 compression algorithm
    #include "external/lz4.c"               // LZ4 compression algorithm implementation
#endif
#if defined(RRES_SUPPORT_ENCRYPTION_MONOCYPHER)
    #include "external/monocypher.h"        // Encryption algorithms
    #include "external/monocypher.c"        // Encryption algorithms implementation
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char *password = NULL;

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------

// Load simple data chunks that are later required by multi-chunk resources
static void *rresLoadDataChunkRaw(rresResourceChunk chunk);         // Load chunk: RRES_DATA_RAW
static char *rresLoadDataChunkText(rresResourceChunk chunk);        // Load chunk: RRES_DATA_TEXT
static Image rresLoadDataChunkImage(rresResourceChunk chunk);       // Load chunk: RRES_DATA_IMAGE

// Unpack compressed/encrypted data from chunk
// NOTE: Function return 0 on success or other value on failure
static int rresUnpackDataChunk(rresResourceChunk *chunk);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load raw data from rres resource
void *rresLoadRaw(rresResource rres, int *size)
{
    void *data = NULL;

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_RAW))
    {
        data = rresLoadDataChunkRaw(rres.chunks[0]);
    }

    return data;
}

// Load text data from rres resource
// NOTE: Text must be NULL terminated
char *rresLoadText(rresResource rres)
{
    char *text = NULL;

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_TEXT))
    {
        text = rresLoadDataChunkText(rres.chunks[0]);
    }

    return text;
}

// Load Image data from rres resource
Image rresLoadImage(rresResource rres)
{
    Image image = { 0 };

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_IMAGE))
    {
        image = rresLoadDataChunkImage(rres.chunks[0]);
    }

    return image;
}

// Load Wave data from rres resource
Wave rresLoadWave(rresResource rres)
{
    Wave wave = { 0 };

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_WAVE))
    {
        if ((chunk.compType > 0) || (chunk.cipherType > 0)) rresUnpackDataChunk(&rres.chunks[0]);

        wave.frameCount = rres.chunks[0].props[0];
        wave.sampleRate = rres.chunks[0].props[1];
        wave.sampleSize = rres.chunks[0].props[2];
        wave.channels = rres.chunks[0].props[3];

        unsigned int size = wave.frameCount*wave.sampleSize/8;
        wave.data = RL_CALLOC(size);
        memcpy(wave.data, rres.chunks[0].data, size);
    }

    return wave;
}

// Load Font data from rres resource
Font rresLoadFont(rresResource rres)
{
    Font font = { 0 };

    // Font resource consist of (2) chunks:
    //  - RRES_DATA_GLYPH_INFO: Basic font and glyphs properties/data
    //  - RRES_DATA_IMAGE: Image atlas for the font characters
    if (rres.count >= 2)
    {
        if (rres.chunks[0].type == RRES_DATA_GLYPH_INFO)
        {
            if ((chunk.compType > 0) || (chunk.cipherType > 0)) rresUnpackDataChunk(&rres.chunks[0]);

            // Load font basic properties from chunk[0]
            font.baseSize = rres.chunks[0].props[0];           // Base size (default chars height)
            font.glyphCount = rres.chunks[0].props[1];         // Number of characters (glyphs)
            font.glyphPadding = rres.chunks[0].props[2];      // Padding around the chars

            font.recs = (Rectangle *)RL_CALLOC(font.glyphCount*sizeof(Rectangle));
            font.glyphs = (GlyphInfo *)RL_CALLOC(font.glyphCount*sizeof(GlyphInfo));

            for (int i = 0; i < font.glyphCount; i++)
            {
                // Font glyphs info comes as a data blob
                font.recs[i].x = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].x;
                font.recs[i].y = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].y;
                font.recs[i].width = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].width;
                font.recs[i].height = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].height;

                font.glyphs[i].value = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].value;
                font.glyphs[i].offsetX = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].offsetX;
                font.glyphs[i].offsetY = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].offsetY;
                font.glyphs[i].advanceX = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].advanceX;

                // NOTE: font.glyphs[i].image is not loaded
            }
        }

        // Load font image chunk
        if (rres.chunks[1].type == RRES_DATA_IMAGE)
        {
            Image image = rresLoadDataChunkImage(rres.chunks[1]);
            font.texture = LoadTextureFromImage(image);
            UnloadImage(image);
        }
    }

    return font;
}

// Load Mesh data from rres resource
Mesh rresLoadMesh(rresResource rres)
{
    Mesh mesh = { 0 };

    mesh.vertexCount = rres.chunks[0].props[0];

    // TODO: Make sure all rres.chunks[i] vertexCount are the same?

    // Mesh resource consist of (n) chunks:
    for (int i = 0; i < rres.count; i++)
    {
        if (rres.chunks[i].type == RRES_DATA_VERTEX)
        {
            switch (rres.chunks[i].props[1])
            {
                case RRES_VERTEX_ATTRIBUTE_POSITION: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_FLOAT) mesh.vertices = (float *)rres.chunks[i].data; break;
                case RRES_VERTEX_ATTRIBUTE_TEXCOORD1: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_FLOAT) mesh.texcoords = (float *)rres.chunks[i].data; break;
                case RRES_VERTEX_ATTRIBUTE_TEXCOORD2: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_FLOAT) mesh.texcoords2 = (float *)rres.chunks[i].data; break;
                case RRES_VERTEX_ATTRIBUTE_TEXCOORD3: break;    // Not supported by raylib mesh format
                case RRES_VERTEX_ATTRIBUTE_TEXCOORD4: break;    // Not supported by raylib mesh format
                case RRES_VERTEX_ATTRIBUTE_NORMAL: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_FLOAT) mesh.normals = (float *)rres.chunks[i].data; break;
                case RRES_VERTEX_ATTRIBUTE_TANGENT: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_FLOAT) mesh.tangents = (float *)rres.chunks[i].data; break;
                case RRES_VERTEX_ATTRIBUTE_COLOR: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_BYTE) mesh.colors = (unsigned char *)rres.chunks[i].data; break;
                case RRES_VERTEX_ATTRIBUTE_INDEX: if (rres.chunks[i].props[2] == RRES_VERTEX_FORMAT_SHORT) mesh.indices = (unsigned short *)rres.chunks[i].data; break;
                default: break;
            }
        }
    }

    return mesh;
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------

// Load data chunk: RRES_DATA_RAW
// NOTE: This chunk can be used raw files embedding or other binary blobs
static void *rresLoadDataChunkRaw(rresResourceChunk chunk)
{
    void *rawData = NULL;

    if (chunk.type == RRES_DATA_RAW)
    {
        if ((chunk.compType > 0) || (chunk.cipherType > 0)) rresUnpackDataChunk(&chunk);

        rawData = RL_CALLOC(chunk.props[0], 1);
        memcpy(rawData, chunk.data, chunk.props[0]);
    }

    return rawData;
}

// Load data chunk: RRES_DATA_TEXT
// NOTE: This chunk can be used for shaders or other text data elements (materials?)
static char *rresLoadDataChunkText(rresResourceChunk chunk)
{
    void *text = NULL;

    if (chunk.type == RRES_DATA_TEXT)
    {
        if ((chunk.compType > 0) || (chunk.cipherType > 0)) rresUnpackDataChunk(&chunk);

        text = (char *)RL_CALLOC(chunk.props[0] + 1, 1);    // We add NULL terminator, just in case
        memcpy(text, chunk.data, chunk.props[0]);
    }

    return text;
}

// Load data chunk: RRES_DATA_IMAGE
// NOTE: Many data types use images data in some way (font, material...)
static Image rresLoadDataChunkImage(rresResourceChunk chunk)
{
    Image image = { 0 };

    if (chunk.type == RRES_DATA_IMAGE)
    {
        if ((chunk.compType > 0) || (chunk.cipherType > 0)) rresUnpackDataChunk(&chunk);

        image.width = chunk.props[0];
        image.height = chunk.props[1];
        image.mipmaps = chunk.props[3];

        // WARNING: rresPixelFormat enum matches raylib PixelFormat enum values but
        // this could require some format assignment -> TODO
        image.format = chunk.props[2];

        // NOTE: Image data size can be computed from image properties
        unsigned int size = GetPixelDataSize(image.width, image.height, image.format);
        image.data = RL_CALLOC(size);
        memcpy(image.data, chunk.data, size);

        // TODO: Consider mipmaps data!
    }

    return image;
}

// Unpack compressed/encrypted data from chunk
// NOTE: Function return 0 on success or other value on failure
static int rresUnpackDataChunk(rresResourceChunk *chunk)
{
    int result = 0;

    // Result error codes:
    //  0 - No error, decompression/decryption successful
    //  1 - Encryption algorithm not supported
    //  2 - Invalid password on decryption
    //  3 - Compression algorithm not supported
    //  4 - Error on data decompression

    // TODO: Decompress and decrypt data as required (only rrespacker supported formats)

    // STEP 1. Decrypt message if encrypted
    if (chunk->cipherType == RRES_CIPHER_XCHACHA20_POLY1305)    // rrespacker supported encryption
    {
#if defined(RRES_SUPPORT_ENCRYPTION_MONOCYPHER)
        // TODO.
#else
        result = 1;
#endif
    }
    else result = 1;

    // STEP 2: Decompress data if compressed
    if (chunk->compType == RRES_COMP_DEFLATE)
    {
        // TODO.
    }
    else if (chunk->compType == RRES_COMP_LZ4)
    {
#if defined(RRES_SUPPORT_COMPRESSION_LZ4)
        // TODO.
#else
        result = 3;
#endif
    }
    else if (chunk->compType == RRES_COMP_QOI)
    {
        // TODO.
    }
    else result = 3;

    return result;
}

// Set password to be used in case of encrypted data found
void rresSetCipherPassword(const char *pass)
{
    password = pass;
}

#endif // RRES_RAYLIB_IMPLEMENTATION
