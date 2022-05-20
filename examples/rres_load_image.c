/*******************************************************************************************
*
*   raylib [rres] example - rres load image
*
*   This example has been created using raylib 4.1-dev (www.raylib.com) and rres 1.0
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2022 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define RRES_IMPLEMENTATION
#include "../src/rres.h"              // Required to read rres data chunks

#define RRES_RAYLIB_IMPLEMENTATION
#define RRES_SUPPORT_COMPRESSION_LZ4
#define RRES_SUPPORT_ENCRYPTION_AES
#define RRES_SUPPORT_ENCRYPTION_XCHACHA20
#include "../src/rres-raylib.h"       // Required to map rres data chunks into raylib structs

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 384;
    const int screenHeight = 512;

    InitWindow(screenWidth, screenHeight, "raylib [rres] example - rres load image");
    
    Texture2D texture = { 0 };          // Texture to load our image data
    
    // Load central directory from .rres file (if available)
    rresCentralDir dir = rresLoadCentralDirectory("resources.rres");
    
    // Get resource id from original fileName (stored in centra directory)
    unsigned int id = rresGetResourceId(dir, "resources/images/fudesumi.png");
    
    // Load resource chunk from file providing the id
    rresResourceChunk chunk = rresLoadResourceChunk("resources.rres", id);
    
    // Decompress/decipher resource data (if required)
    result = UnpackResourceChunk(&chunk);
    
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