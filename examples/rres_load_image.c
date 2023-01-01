/*******************************************************************************************
*
*   rres example - rres load image
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

#define RRES_RAYLIB_IMPLEMENTATION
#define RRES_SUPPORT_COMPRESSION_LZ4
#define RRES_SUPPORT_ENCRYPTION_AES
#define RRES_SUPPORT_ENCRYPTION_XCHACHA20
#include "../src/rres-raylib.h"       // Required to map rres data chunks into raylib structs

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 384;
    const int screenHeight = 512;

    InitWindow(screenWidth, screenHeight, "rres example - rres load image");
    
    Texture2D texture = { 0 };          // Texture to load our image data
    
    // Load central directory from .rres file (if available)
    rresCentralDir dir = rresLoadCentralDirectory("resources.rres");
    
    // Get resource id from original fileName (stored in centra directory)
    unsigned int id = rresGetResourceId(dir, "resources/images/fudesumi.png");
    
    // Setup password to load encrypted data (if required)
    rresSetCipherPassword("password12345");
    
    // Load resource chunk from file providing the id
    rresResourceChunk chunk = rresLoadResourceChunk("resources.rres", id);
    
    // Decompress/decipher resource data (if required)
    int result = UnpackResourceChunk(&chunk);
    
    if (result == RRES_SUCCESS)         // Data decompressed/decrypted successfully
    {
        // Load image data from resource chunk
        Image image = LoadImageFromResource(chunk);
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
    }

    rresUnloadResourceChunk(chunk);     // Unload resource chunk
    rresUnloadCentralDirectory(dir);    // Unload central directory, not required any more

    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            DrawTexture(texture, 0, 0, WHITE);  // Draw loaded texture

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(texture);             // Unload texture (VRAM)
    CloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
