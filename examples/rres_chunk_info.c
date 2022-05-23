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
    rresResourceChunkInfo *chunks = rresLoadResourceChunkInfoAll("resources.rres", &chunkCount);
    
    // TODO: Display info properly
    
    return 0;
}