/*******************************************************************************************
*
*   rres example - rres create file
*
*   This example has been created using rres 1.0 (github.com/raysan5/rres)
*   This example uses raylib 4.1-dev (www.raylib.com) to display loaded data
*
*
*   LICENSE: MIT
*
*   Copyright (c) 2022-2023 Ramon Santamaria (@raysan5)
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

#include "raylib.h"

#define RRES_IMPLEMENTATION
#include "../src/rres.h"              // Required to read rres data chunks


// Load a continuous data buffer from rresResourceChunkData struct
static unsigned char *LoadDataBuffer(rresResourceChunkData data, unsigned int rawSize);
static void UnloadDataBuffer(unsigned char *buffer);   // Unload data buffer

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    FILE *rresFile = fopen("myresources.rres", "wb");

    // Define rres file header
    // NOTE: We are loading 4 files that generate 5 resource chunks to save in rres
    rresFileHeader header = {
        .id[0] = 'r',           // File identifier: rres
        .id[1] = 'r',           // File identifier: rres
        .id[2] = 'e',           // File identifier: rres
        .id[3] = 's',           // File identifier: rres
        .version = 100,         // File version: 100 for version 1.0
        .chunkCount = 5,        // Number of resource chunks in the file (MAX: 65535)
        .cdOffset = 0,          // Central Directory offset in file (0 if not available)
        .reserved = 0           // <reserved>
    };
    
    // Write rres file header
    fwrite(&header, sizeof(rresFileHeader), 1, rresFile);
    
    rresResourceChunkInfo chunkInfo = { 0 };    // Chunk info
    rresResourceChunkData chunkData = { 0 };    // Chunk data
    
    unsigned char *buffer = NULL;
    
    // File 01: Text file -> One resource chunk: TEXT
    //---------------------------------------------------------------------------------
    // Load file data
    char *text = LoadFileText("resources/text_data.txt");
    unsigned int rawSize = strlen(text);
    
    // Define chunk info: TEXT
    chunkInfo.type[0] = 'T';         // Resource chunk type (FourCC)
    chunkInfo.type[1] = 'E';         // Resource chunk type (FourCC)
    chunkInfo.type[2] = 'X';         // Resource chunk type (FourCC)
    chunkInfo.type[3] = 'T';         // Resource chunk type (FourCC)
    
    // Resource chunk identifier (generated from filename CRC32 hash)
    chunkInfo.id = rresComputeCRC32("resources/text_data.txt", strlen("resources/text_data.txt"));
    
    chunkInfo.compType = RRES_COMP_NONE,     // Data compression algorithm
    chunkInfo.cipherType = RRES_CIPHER_NONE, // Data encription algorithm
    chunkInfo.flags = 0,             // Data flags (if required)
    chunkInfo.baseSize = 5*sizeof(unsigned int) + rawSize;   // Data base size (uncompressed/unencrypted)
    chunkInfo.packedSize = chunkInfo.baseSize; // Data chunk size (compressed/encrypted + custom data appended)
    chunkInfo.nextOffset = 0,        // Next resource chunk global offset (if resource has multiple chunks)
    chunkInfo.reserved = 0,          // <reserved>
    
    // Define chunk data: TEXT
    chunkData.propCount = 4;
    chunkData.props = (unsigned int *)RRES_CALLOC(chunkData.propCount, sizeof(unsigned int));
    chunkData.props[0] = rawSize;    // props[0]:size (bytes)
    chunkData.props[1] = RRES_TEXT_ENCODING_UNDEFINED;  // props[1]:rresTextEncoding
    chunkData.props[2] = RRES_CODE_LANG_UNDEFINED;      // props[2]:rresCodeLang
    chunkData.props[3] = 0x0409;     // props[3]:cultureCode: en-US: English - United States
    chunkData.raw = text;
    
    // Get a continuous data buffer from chunkData
    buffer = LoadDataBuffer(chunkData, rawSize);
    
    // Compute data chunk CRC32 (propCount + props[] + data)
    chunkInfo.crc32 = rresComputeCRC32(buffer, chunkInfo.packedSize);

    // Write resource chunk into rres file
    fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
    fwrite(buffer, 1, chunkInfo.packedSize, rresFile);
    
    // Free required memory
    memset(&chunkInfo, 0, sizeof(rresResourceChunkInfo));
    RRES_FREE(chunkData.props);
    UnloadDataBuffer(buffer);
    UnloadFileText(text);
    //---------------------------------------------------------------------------------
    
    // File 02: Image file -> One resource chunk: IMGE
    //---------------------------------------------------------------------------------
    // Load file data
    Image image = LoadImage("resources/images/fudesumi.png");
    rawSize = GetPixelDataSize(image.width, image.height, image.format);
    
    // Define chunk info: IMGE
    chunkInfo.type[0] = 'I';         // Resource chunk type (FourCC)
    chunkInfo.type[1] = 'M';         // Resource chunk type (FourCC)
    chunkInfo.type[2] = 'G';         // Resource chunk type (FourCC)
    chunkInfo.type[3] = 'E';         // Resource chunk type (FourCC)
    
    // Resource chunk identifier (generated from filename CRC32 hash)
    chunkInfo.id = rresComputeCRC32("resources/images/fudesumi.png", strlen("resources/images/fudesumi.png"));
    
    chunkInfo.compType = RRES_COMP_NONE,     // Data compression algorithm
    chunkInfo.cipherType = RRES_CIPHER_NONE, // Data encription algorithm
    chunkInfo.flags = 0,             // Data flags (if required)
    chunkInfo.baseSize = 5*sizeof(unsigned int) + rawSize;   // Data base size (uncompressed/unencrypted)
    chunkInfo.packedSize = chunkInfo.baseSize; // Data chunk size (compressed/encrypted + custom data appended)
    chunkInfo.nextOffset = 0,        // Next resource chunk global offset (if resource has multiple chunks)
    chunkInfo.reserved = 0,          // <reserved>
    
    // Define chunk data: IMGE
    chunkData.propCount = 4;
    chunkData.props = (unsigned int *)RRES_CALLOC(chunkData.propCount, sizeof(unsigned int));
    chunkData.props[0] = image.width;      // props[0]:width
    chunkData.props[1] = image.height;     // props[1]:height
    chunkData.props[2] = image.format;     // props[2]:rresPixelFormat
                                           // NOTE: rresPixelFormat matches raylib PixelFormat enum, 
    chunkData.props[3] = image.mipmaps;    // props[3]:mipmaps                
    chunkData.raw = image.data;
    
    // Get a continuous data buffer from chunkData
    buffer = LoadDataBuffer(chunkData, rawSize);
    
    // Compute data chunk CRC32 (propCount + props[] + data)
    chunkInfo.crc32 = rresComputeCRC32(buffer, chunkInfo.packedSize);

    // Write resource chunk into rres file
    fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
    fwrite(buffer, 1, chunkInfo.packedSize, rresFile);
    
    // Free required memory
    memset(&chunkInfo, 0, sizeof(rresResourceChunkInfo));
    RRES_FREE(chunkData.props);
    UnloadDataBuffer(buffer);
    UnloadImage(image);
    //---------------------------------------------------------------------------------
    
    // File 03: Wave file -> One resource chunk: WAVE
    //---------------------------------------------------------------------------------
    // Load file data
    Wave wave = LoadWave("resources/audio/coin.wav");
    rawSize = wave.frameCount*wave.channels*(wave.sampleSize/8);
    
    // Define chunk info: WAVE
    chunkInfo.type[0] = 'W';         // Resource chunk type (FourCC)
    chunkInfo.type[1] = 'A';         // Resource chunk type (FourCC)
    chunkInfo.type[2] = 'V';         // Resource chunk type (FourCC)
    chunkInfo.type[3] = 'E';         // Resource chunk type (FourCC)
    
    // Resource chunk identifier (generated from filename CRC32 hash)
    chunkInfo.id = rresComputeCRC32("resources/audio/coin.wav", strlen("resources/audio/coin.wav"));
    
    chunkInfo.compType = RRES_COMP_NONE,     // Data compression algorithm
    chunkInfo.cipherType = RRES_CIPHER_NONE, // Data encription algorithm
    chunkInfo.flags = 0,             // Data flags (if required)
    chunkInfo.baseSize = 5*sizeof(unsigned int) + rawSize;   // Data base size (uncompressed/unencrypted)
    chunkInfo.packedSize = chunkInfo.baseSize; // Data chunk size (compressed/encrypted + custom data appended)
    chunkInfo.nextOffset = 0,        // Next resource chunk global offset (if resource has multiple chunks)
    chunkInfo.reserved = 0,          // <reserved>
    
    // Define chunk data: WAVE
    chunkData.propCount = 4;
    chunkData.props = (unsigned int *)RRES_CALLOC(chunkData.propCount, sizeof(unsigned int));
    chunkData.props[0] = wave.frameCount;      // props[0]:frameCount
    chunkData.props[1] = wave.sampleRate;      // props[1]:sampleRate
    chunkData.props[2] = wave.sampleSize;      // props[2]:sampleSize
    chunkData.props[3] = wave.channels;        // props[3]:channels    
    chunkData.raw = wave.data;
    
    // Get a continuous data buffer from chunkData
    buffer = LoadDataBuffer(chunkData, rawSize);
    
    // Compute data chunk CRC32 (propCount + props[] + data)
    chunkInfo.crc32 = rresComputeCRC32(buffer, chunkInfo.packedSize);

    // Write resource chunk into rres file
    fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
    fwrite(buffer, 1, chunkInfo.packedSize, rresFile);
    
    // Free required memory
    memset(&chunkInfo, 0, sizeof(rresResourceChunkInfo));
    RRES_FREE(chunkData.props);
    UnloadDataBuffer(buffer);
    UnloadWave(wave);
    //---------------------------------------------------------------------------------
    
    // File 04: Font file -> Two resource chunks: FNTG, IMGE
    //---------------------------------------------------------------------------------
    // Load file data
    Font font = LoadFont("resources/fonts/pixantiqua.ttf");
    
    // TODO.
    
    // Free required memory
    //memset(&chunkInfo, 0, sizeof(rresResourceChunkInfo));
    //RRES_FREE(data.props);
    //UnloadDataBuffer(buffer);
    UnloadFont(font);
    //---------------------------------------------------------------------------------
    
    fclose(rresFile);

    return 0;
}

// Load a continuous data buffer from rresResourceChunkData struct
static unsigned char *LoadDataBuffer(rresResourceChunkData data, unsigned int rawSize)
{
    unsigned char *buffer = (unsigned char *)RRES_CALLOC((data.propCount + 1)*sizeof(unsigned int) + rawSize, 1);
    
    memcpy(buffer, &data.propCount, sizeof(unsigned int));
    for (int i = 0; i < data.propCount; i++) memcpy(buffer + (i + 1)*sizeof(unsigned int), &data.props[i], sizeof(unsigned int));
    memcpy(buffer + (data.propCount + 1)*sizeof(unsigned int), data.raw, rawSize);
    
    return buffer;
}

// Unload data buffer
static void UnloadDataBuffer(unsigned char *buffer)
{
    RRES_FREE(buffer);
}
