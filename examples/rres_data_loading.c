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

#include <stdio.h>

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [rres] example - rres data loading");

    InitAudioDevice();

    // TEST 00: RRES_DATA_DIRECTORY: OK!!!
    rresCentralDir dir = rresLoadCentralDirectory("resources.rres");

    // Check if central directory is available
    // NOTE: CDIR is not mandatory, resources are referenced by its id
    if (dir.count == 0) TraceLog(LOG_WARNING, "No central directory available in the file");
    else
    {
        // List all files contained on central directory
        for (int i = 0; i < dir.count; i++)
        {
            TraceLog(LOG_INFO, "FILE: [%08X] Entry (0x%x): %s (len: %i)", dir.entries[i].id, dir.entries[i].offset, dir.entries[i].fileName, dir.entries[i].fileNameLen);
        }
    }

    // Load content from rres file
    rresData rres = { 0 };

    // TEST 01: RRES_DATA_RAW
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "C:\\GitHub\\rres\\examples\\resources\\image.png.raw"));
    unsigned int dataSize = 0;
    void *data = rresLoadRaw(rres, &dataSize);
    rresUnloadData(rres);

    FILE *rawFile = fopen("export_image.png", "wb");
    fwrite(data, dataSize, 1, rawFile);
    fclose(rawFile);

    // TEST 02: RRES_DATA_TEXT -> OK!!!
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "C:\\GitHub\\rres\\examples\\resources\\text_data.txt"));
    char *text = rresLoadText(rres);
    rresUnloadData(rres);

    // TEST 03: RRES_DATA_IMAGE -> OK!!!
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "C:\\GitHub\\rres\\examples\\resources\\images\\fudesumi.png"));
    Image image = rresLoadImage(rres);
    rresUnloadData(rres);

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    // TEST 03: RRES_DATA_WAVE -> OK!!!
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "C:\\GitHub\\rres\\examples\\resources\\audio\\target.ogg"));
    Wave wave = rresLoadWave(rres);
    rresUnloadData(rres);

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);

    // TEST 04: RRES_DATA_FONT_INFO (multichunk!)
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "C:\\GitHub\\rres\\examples\\resources\\fonts\\pixantiqua.ttf"));
    Font font = rresLoadFont(rres);
    rresUnloadData(rres);

    // TEST 05: RRES_DATA_VERTEX (multichunk!)
    rres = rresLoadData("resources.rres", rresGetIdFromFileName(dir, "C:\\GitHub\\rres\\examples\\resources\\models\\castle.obj"));
    Mesh mesh = rresLoadMesh(rres);
    rresUnloadData(rres);

    Model model = LoadModelFromMesh(mesh);

    // Unload central directory info, not required any more
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
            DrawTextEx(font, text, (Vector2){ 300, 20 }, 40, 2, BLUE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    MemFree(data);              // Unload raw data
    MemFree(text);              // Unload text data
    UnloadTexture(texture);     // Unload texture (VRAM)
    UnloadSound(sound);
    UnloadFont(font);
    UnloadModel(model);

    CloseAudioDevice();         // Close audio device

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}