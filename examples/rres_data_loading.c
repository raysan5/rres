/*******************************************************************************************
*
*   rres example - rres data loading
*
*   This example has been created using rres 1.0 (github.com/raysan5/rres)
*   This example uses raylib 4.2 (www.raylib.com) to display loaded data
*
*
*   LICENSE: MIT
*
*   Copyright (c) 2021-2023 Ramon Santamaria (@raysan5)
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
#include "../src/rres.h"            // Required to read rres data chunks

#define RRES_RAYLIB_IMPLEMENTATION
#define RRES_SUPPORT_COMPRESSION_LZ4
#define RRES_SUPPORT_ENCRYPTION_AES
#define RRES_SUPPORT_ENCRYPTION_XCHACHA20
#include "../src/rres-raylib.h"     // Required to map rres data chunks into raylib structs

#include <stdio.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 384;
    const int screenHeight = 512;

    InitWindow(screenWidth, screenHeight, "rres example - rres data loading");

    void *data = NULL;              // Store RRES_DATA_RAW loaded data
    char *text = NULL;              // Store RRES_DATA_TEXT loaded data
    Texture2D texture = { 0 };      // Store RRES_DATA_IMAGE loaded data -> LoadTextureFromImage()
    Sound sound = { 0 };            // Store RRES_DATA_WAVE loaded data -> LoadSoundFromWave()
    Font font = { 0 };              // Store RRES_DATA_FONT_GLYPHS + RRES_DATA_IMAGE

    // Load content from rres file
    rresResourceChunk chunk = { 0 }; // Single resource chunk
    rresResourceMulti multi = { 0 }; // Multiple resource chunks

    InitAudioDevice();              // Initialize audio device, useful for audio testing

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Dropped files logic
        //----------------------------------------------------------------------------------
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            if (IsFileExtension(droppedFiles.paths[0], ".rres"))
            {
                int result = 0;     // Result of data unpacking

                // TEST 01: Load rres Central Directory (RRES_DATA_DIRECTORY)
                //------------------------------------------------------------------------------------------------------
                rresCentralDir dir = rresLoadCentralDirectory(droppedFiles.paths[0]);

                // NOTE: By default central directory is never compressed/encrypted

                // Check if central directory is available
                // NOTE: CDIR is not mandatory, resources are referenced by its id
                if (dir.count == 0) TraceLog(LOG_WARNING, "No central directory available in the file");
                else
                {
                    // List all files contained on central directory
                    for (unsigned int i = 0; i < dir.count; i++)
                    {
                        TraceLog(LOG_INFO, "RRES: CDIR: File entry %03i: %s | Resource(s) id: 0x%08x | Offset: 0x%08x", i + 1, dir.entries[i].fileName, dir.entries[i].id, dir.entries[i].offset);
                    
                        // TODO: List contained resource chunks info
                        //rresResourceChunkInfo info = rresGetResourceChunkInfo(droppedFiles.paths[0], dir.entries[i]);
                    }
                }
                //------------------------------------------------------------------------------------------------------
                /*
                // TEST 02: Loading raw data (RRES_DATA_RAW)
                //------------------------------------------------------------------------------------------------------
                chunk = rresLoadResourceChunk(droppedFiles.paths[0], rresGetResourceId(dir, "resources/image.png.raw"));
                result = UnpackResourceChunk(&chunk);               // Decompres/decipher resource data (if required)

                if (result == 0)    // Data decompressed/decrypted successfully
                {
                    unsigned int dataSize = 0;
                    data = LoadDataFromResource(chunk, &dataSize);  // Load raw data, must be freed at the end

                    if ((data != NULL) && (dataSize > 0))
                    {
                        FILE *rawFile = fopen("export_data.raw", "wb");
                        fwrite(data, dataSize, 1, rawFile);
                        fclose(rawFile);
                    }
                }

                rresUnloadResourceChunk(chunk);
                //------------------------------------------------------------------------------------------------------

                // TEST 03: Load text data (RRES_DATA_TEXT)
                //------------------------------------------------------------------------------------------------------
                chunk = rresLoadResourceChunk(droppedFiles.paths[0], rresGetResourceId(dir, "resources/text_data.txt"));
                result = UnpackResourceChunk(&chunk);       // Decompres/decipher resource data (if required)

                if (result == 0)    // Data decompressed/decrypted successfully
                {
                    text = LoadTextFromResource(chunk);     // Load text data, must be freed at the end
                }

                rresUnloadResourceChunk(chunk);
                //------------------------------------------------------------------------------------------------------
                */
                
                // TEST 04: Load image data (RRES_DATA_IMAGE)
                //------------------------------------------------------------------------------------------------------
                chunk = rresLoadResourceChunk(droppedFiles.paths[0], rresGetResourceId(dir, "fudesumi.png"));
                result = UnpackResourceChunk(&chunk);       // Decompres/decipher resource data (if required)

                if (result == 0)    // Data decompressed/decrypted successfully
                {
                    Image image = LoadImageFromResource(chunk);
                    if (image.data != NULL)
                    {
                        texture = LoadTextureFromImage(image);
                        UnloadImage(image);
                    }
                }

                rresUnloadResourceChunk(chunk);
                //------------------------------------------------------------------------------------------------------

                // TEST 05: Load wave data (RRES_DATA_WAVE)
                //------------------------------------------------------------------------------------------------------
                chunk = rresLoadResourceChunk(droppedFiles.paths[0], rresGetResourceId(dir, "tanatana.ogg"));
                result = UnpackResourceChunk(&chunk);       // Decompres/decipher resource data (if required)

                if (result == 0)    // Data decompressed/decrypted successfully
                {
                    Wave wave = LoadWaveFromResource(chunk);
                    sound = LoadSoundFromWave(wave);
                    UnloadWave(wave);
                }

                rresUnloadResourceChunk(chunk);
                //------------------------------------------------------------------------------------------------------
                
                // TEST 06: Load font data, multiples chunks (RRES_DATA_FONT_GLYPHS + RRE_DATA_IMAGE)
                //------------------------------------------------------------------------------------------------------
                multi = rresLoadResourceMulti(droppedFiles.paths[0], rresGetResourceId(dir, "pixantiqua.ttf"));
                for (int i = 0; i < multi.count; i++)
                {
                    result = UnpackResourceChunk(&multi.chunks[i]);   // Decompres/decipher resource data (if required)
                    if (result != 0) break;
                }

                if (result == 0)    // All resources data decompressed/decrypted successfully
                {
                    font = LoadFontFromResource(multi);
                }
                
                rresUnloadResourceMulti(multi);
                //------------------------------------------------------------------------------------------------------
                
                // Unload central directory info, not required any more
                rresUnloadCentralDirectory(dir);
            }

            UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
        }
        //----------------------------------------------------------------------------------

        // Update
        //----------------------------------------------------------------------------------
        // Play audio loaded from wave from .rres: RRES_DATA_WAVE
        if (IsKeyPressed(KEY_SPACE)) PlaySound(sound);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("rres file loading: drag & drop a .rres file", 10, 10, 10, DARKGRAY);

            // Draw texture loaded from image from .rres: RRES_DATA_IMAGE
            DrawTexture(texture, 0, 0, WHITE);

            // Draw text using font loaded from .rres: RRES_DATA_FONT_GLYPHS + RRES_DATA_IMAGE
            DrawTextEx(font, "THIS IS a TEST!", (Vector2) { 10, 50 }, font.baseSize, 0, RED);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    MemFree(data);              // Unload raw data, using raylib memory allocator (same used by rres-raylib.h)
    MemFree(text);              // Unload text data, using raylib memory allocator (same used by rres-raylib.h)
    UnloadTexture(texture);     // Unload texture (VRAM)
    UnloadSound(sound);         // Unload sound
    UnloadFont(font);           // Unload font

    CloseAudioDevice();         // Close audio device

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}