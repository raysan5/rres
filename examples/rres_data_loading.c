/*******************************************************************************************
*
*   raylib [rres] example - rres data loading
*
*   This example has been created using raylib 3.7 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2021 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define RRES_IMPLEMENTATION
#include "../src/rres.h"

#define RRES_RAYLIB_IMPLEMENTATION
#include "../src/rres-raylib.h"

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [rres] example - rres data loading");
    
    InitAudioDevice();
    
    rresCentralDir dir = rresLoadCentralDirectory("resources.rres");
    
    // Check if central directory is available
    // NOTE: CDIR is not mandatory, resources are referenced by its id
    if (dir.count > 0)
    {
        // List all files contained
        for (int i = 0; i < dir.count; i++)
        {
            TraceLog(LOG_INFO, "FILE: [%08X] Entry (0x%x): %s (len: %i)", dir.entries[i].id, dir.entries[i].offset, dir.entries[i].fileName, dir.entries[i].fileNameLen);
        }
    }        

    // Load content from rres file
    rresData rres = { 0 };
    
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "resources/images/fudesumi.png"));
    Image image = rresLoadImage(rres);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "resources/fonts/pixantiqua.ttf"));
    Font font = rresLoadFont(rres);
    
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "resources/audio/sound.wav"));
    Wave wave = rresLoadWave(rres);
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    
    rresUnloadData(rres);
    rresUnloadCentralDirectory(dir);

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) PlaySound(sound);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            
            DrawText("rres file loading example", 10, 10, 20, DARKGRAY);

            DrawTexture(texture, 20, 20, WHITE);
            DrawTextEx(font, "custom font!", (Vector2){ 100, 100 }, 40, 2, BLUE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(texture);
    UnloadFont(font);
    UnloadSound(sound);
    
    CloseAudioDevice();     // Close audio device
    
    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}