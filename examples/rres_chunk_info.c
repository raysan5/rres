/*******************************************************************************************
*
*   rres example - rres chunk info
*
*   This example has been created using rres 1.0 (github.com/raysan5/rres)
*
*
*   LICENSE: MIT
*
*   Copyright (c) 2022 Ramon Santamaria (@raysan5)
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

#define RRES_IMPLEMENTATION
#include "../src/rres.h"        // Required to read rres data chunks

#include <stdio.h>              // Required for: printf()


static const char *GetCompressionName(int compType);    // Get compression name as a text string
static const char *GetCipherName(int cipherType);       // Get cipher name as a text string

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Load central directory from .rres file (if available)
    rresCentralDir dir = rresLoadCentralDirectory("resources.rres");
    
    if (dir.count == 0) printf("WARNING: Central Directory not available\n"); // No central directory
    
    // NOTE: With no CDIR we can still load the contained resources info,
    // but we can't know the original input files that generated the resource chunks
    
    // Load ALL resource chunks info from .rres file
    unsigned int chunkCount = 0;
    rresResourceChunkInfo *infos = rresLoadResourceChunkInfoAll("resources.rres", &chunkCount);

    unsigned int prevId = 0;
    
    // Display resource chunks info
    // NOTE: Central Directory relates input files to rres resource chunks,
    // some input files could generate multiple rres resource chunks (Font files)
    for (int i = 0; i < chunkCount; i++)
    {
        for (int j = 0; j < dir.count; j++)
        {
            if ((infos[i].id == dir.entries[j].id) && (infos[i].id != prevId))
            {
                printf("Input File: %s\n", dir.entries[j].fileName);
                printf("Resource(s) Offset: 0x%08x\n", dir.entries[j].offset);
                prevId = dir.entries[j].id;
                break;
            }
        }

        printf("    Resource Chunk: %c%c%c%c\n", infos[i].type[0], infos[i].type[1], infos[i].type[2], infos[i].type[3]);
        printf("       > id:            0x%08x\n", infos[i].id);
        printf("       > compType:      %s (%i)\n", GetCompressionName((int)infos[i].compType), (int)infos[i].compType);
        printf("       > cipherType:    %s (%i)\n", GetCipherName((int)infos[i].cipherType), (int)infos[i].cipherType);
        printf("       > baseSize:      %i\n", infos[i].baseSize);
        printf("       > packedSize:    %i (%i%%)\n", infos[i].packedSize, (int)(((float)infos[i].packedSize/infos[i].baseSize)*100)); // Get compression ratio
        printf("       > nextOffset:    %i\n", infos[i].nextOffset);
        printf("       > CRC32:         0x%08x\n", infos[i].crc32);
    }

    // Free allocated memory for chunks info
    RRES_FREE(infos);

    return 0;
}

// Get compression name as a text string
static const char *GetCompressionName(int compType)
{
    // Map compression type: NONE;DEFLATE;LZ4;QOI
    if (compType == RRES_COMP_NONE) return "none";
    else if (compType == RRES_COMP_DEFLATE) return "DEFLATE";
    else if (compType == RRES_COMP_LZ4) return "LZ4";
    else if (compType == RRES_COMP_QOI) return "QOI";
    else return "Undefined";
}

// Get cipher name as a text string
static const char *GetCipherName(int cipherType)
{
    // Map cipher type: NONE;AES;XCHACHA20
    if (cipherType == RRES_CIPHER_NONE) return "none";
    else if (cipherType == RRES_CIPHER_AES) return "AES-256";
    else if (cipherType == RRES_CIPHER_XCHACHA20_POLY1305) return "XChaCha20";
    else return "Undefined";
}