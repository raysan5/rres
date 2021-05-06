/**********************************************************************************************
*
*   rREM v1.0 - A simple and easy to use resource packer and embedder
*
*   DESCRIPTION:
*       Tool for embedding resources (images, text, sounds...) into a file; possible uses:
*         - Embedding into a resource file (.rres)
*         - Directly embedding into executable program (.exe)
*
*   rrem creates a .rres resource with embedded files and a .h header to access embedded data.
*
*   DEPENDENCIES:
*       raylib 3.7              - Windowing/input management, fileformats loading and drawing.
*       raygui 2.7              - Immediate-mode GUI controls.
*       tinyfiledialogs 3.4.3   - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs.
*
*   COMPILATION (Windows - MinGW):
*       gcc -o rrem.exe rrem.c external/miniz.c -s rrem.rc.data -Iexternal /
*           -lraylib -lopengl32 -lgdi32 -std=c99 -Wall -mwindows
*
*   DEVELOPERS:
*       Ramon Santamaria (@raysan5):   Developer, supervisor, updater and maintainer.
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2021 Ramon Santamaria (@raysan5)
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
***********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS        // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>  // Emscripten library - LLVM to JavaScript compiler
#endif

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raygui.h"                     // Required for: IMGUI controls

#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again
#undef RICONS_IMPLEMENTATION            // Avoid including raygui icons data again

//#define GUI_WINDOW_ABOUT_IMPLEMENTATION
//#include "gui_window_about.h"           // GUI: About Window

//#define GUI_FILE_DIALOGS_IMPLEMENTATION
//#include "gui_file_dialogs.h"           // GUI: File Dialogs

#define RRES_IMPLEMENTATION
#include "../../../src/rres.h"          // Required for: rRES file management

#include "external/dirent.h"            // Required for: DIR, opendir(), readdir()

#include <stdlib.h>                     // Required for:
#include <stdio.h>                      // Required for:
#include <string.h>                     // Required for: strlen(), strrchr(), strcmp()

#define MAX_RESOURCES       512

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#if (!defined(DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
bool __stdcall FreeConsole(void);       // Close console from code (kernel32.lib)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Loaded file required info
// NOTE: Some files translate into multiple resource chunks (Font, Mesh)
typedef struct {
    int fileType;           // FILE_RAW, FILE_TEXT, FILE_IMAGE, FILE_AUDIO, FILE_FONT, FILE_MESH...
    int fileNameLength;
    char fileName[512];
    
    bool forceRaw;          // EDITABLE by users!
    int hashId;             // If no customId provided uses hashId
    int customId;           // EDITABLE by users!
    int compType;           // EDITABLE by users!
    int crypType;           // EDITABLE by users!
    
} FileInfo;
/*
typedef struct {
    // Info required to generate central directory!
    int offset;             // Resource offset in rres file (after file header)
    int fileNameLength;
    char fileName[512];     // Original file name for that resource

    int type;
    int compType;           // EDITABLE by users!
    int crypType;           // EDITABLE by users!
    int compSize;
    int uncompSize;
    int nextOffset;
    int crc32;
} ResChunk;
*/
typedef enum {
    FILE_RAW = 0,
    FILE_TEXT,
    FILE_IMAGE,
    FILE_AUDIO,
    FILE_FONT,
    FILE_MESH,
    //...
} FileType;

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
static const char *toolName = "rREM";
static const char *toolVersion = "1.0";
static const char *toolDescription = "A simple and easy-to-use rres resources packer";

static FileInfo *resFiles = NULL;           // Init to MAX_RESOURCES
static unsigned int fileCount = 0;

static bool saveCentralDir = true;

// Move all variables here as globals for web compilation
static char inFileName[512] = { 0 };        // Input file name (required in case of drag & drop over executable)
static char outFileName[512] = { 0 };       // Output file name (required for file save/export)

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input

static unsigned int ComputeHashId(const char *fileName);    // Compute chunk hash id from filename

static int GetFileType(const char *fileName);               // Get rres file type from fileName extension
static void GenerateRRES(const char *fileName);             // Process all required files data and generate .rres


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
#if !defined(DEBUG)
    //SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
#endif
#if defined(COMMAND_LINE_ONLY)
    ProcessCommandLine(argc, argv);
#else
    // Command-line usage mode
    //--------------------------------------------------------------------------------------
    if (argc > 1)
    {
        // NOTE: In this specific tool we want to just process all files dropped over 
        // executable to generate a rres automatically, but when called from command-line
        // we can attach to every filename some packaging configuration properties!
        ProcessCommandLine(argc, argv);
        return 0;
    }

#if (!defined(DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
    // WARNING (Windows): If program is compiled as Window application (instead of console),
    // no console is available to show output info... solution is compiling a console application
    // and closing console (FreeConsole()) when changing to GUI interface
    //FreeConsole();
#endif

    char currentPath[1024] = { 0 };
    strcpy(currentPath, GetWorkingDirectory());

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 720;
    const int screenHeight = 640;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);      // Window configuration flags
    InitWindow(screenWidth, screenHeight, TextFormat("%s v%s - %s", toolName, toolVersion, toolDescription));
    SetWindowMinSize(640, 480);
    
    resFiles = (FileInfo *)RL_CALLOC(MAX_RESOURCES, sizeof(FileInfo));
    
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (IsFileDropped())
        {
            int dropsCount = 0;
            char **droppedFiles = GetDroppedFiles(&dropsCount);

            // Add dropped files to resources list
            for (int i = 0; i < dropsCount; i++)
            {
                // Check if dropped file is a directory and scan it
                if (DirectoryExists(droppedFiles[i])) 
                {
                    int dirFilesCount = 0;
                    char **files = GetDirectoryFiles(droppedFiles[i], &dirFilesCount);
                    
                    // TODO: WARNING: Directories can contain other directories!!! Also, be careful with '.' and '..'
                    
                    for (int j = 0; j < dirFilesCount; j++)
                    {
                        if (!TextIsEqual(files[j], ".") && !TextIsEqual(files[j], ".."))
                        {
                            resFiles[fileCount].fileType = GetFileType(files[j]);
                            resFiles[fileCount].fileNameLength = TextCopy(resFiles[fileCount].fileName, files[j]);
                            resFiles[fileCount].hashId = ComputeHashId(resFiles[fileCount].fileName);
                            
                            // TODO: Add required resource chunk(s) to list for that file type
                            
                            fileCount++;
                            if (fileCount >= MAX_RESOURCES) break;
                        }
                    }
                    
                    ClearDirectoryFiles();
                }
                else 
                {
                    resFiles[fileCount].fileType = GetFileType(droppedFiles[i]);
                    resFiles[fileCount].fileNameLength = TextCopy(resFiles[fileCount].fileName, droppedFiles[i]);
                    resFiles[fileCount].hashId = ComputeHashId(resFiles[fileCount].fileName);
                    
                    // TODO: Add required resource chunk(s) to list for that file type? -> Required? -> Or do it in processing?
                    // -> We need to configure every chunk independently before processing!
                        // -> On file procesing some fields are filled: compSize, crc32, nextOffset
                        // -> Also, we can not fill the complete central directory info until processed
                    
                    fileCount++;
                    if (fileCount >= MAX_RESOURCES) break;
                }
            }

            ClearDroppedFiles();
        }
        
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            if (fileCount == 0) DrawText("Drop your files to this window!", 100, 40, 20, DARKGRAY);
            else
            {
                DrawText("Dropped files:", 100, 40, 20, DARKGRAY);

                for (int i = 0; i < fileCount; i++)
                {
                    if (i%2 == 0) DrawRectangle(0, 85 + 40*i, GetScreenWidth(), 40, Fade(LIGHTGRAY, 0.5f));
                    else DrawRectangle(0, 85 + 40*i, GetScreenWidth(), 40, Fade(LIGHTGRAY, 0.3f));

                    DrawText(resFiles[i].fileName, 120, 100 + 40*i, 10, GRAY);
                }

                DrawText("Drop new files...", 100, 110 + 40*fileCount, 20, DARKGRAY);
            }
            
            DrawText(TextFormat("%02i", fileCount), 10, 10, 20, MAROON);
            
            if (GuiButton((Rectangle){ 10, GetScreenHeight() - 50, 160, 40 }, "Generate RRES"))
            {
                GenerateRRES("resources.rres");
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RL_FREE(resFiles);     // Free resources files info
    
    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

#endif      // COMMAND_LINE_ONLY

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//--------------------------------------------------------------------------------------------

// Show command line usage info
static void ShowCommandLineInfo(void)
{
    // Some design notes:
    //  - User should be allowed to choose some resource properties: compression/encryption/force-raw-mode/custom-id
    //  - User should be allowed to choose if including a central-directory or not (included by default)
    //  - User should be allowed to choose if filename info goes to central-directory -> maybe in a future...
    
    printf("\n//////////////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                                      //\n");
    printf("// %s v%s - %s //\n", toolName, toolVersion, toolDescription);
    printf("// powered by raylib v3.0 (www.raylib.com) and raygui v2.7                              //\n");
    printf("// more info and bugs-report: github.com/raysan5/rres                                   //\n");
    printf("//                                                                                      //\n");
    printf("// Copyright (c) 2014-2020 Ramon Santamaria (@raysan5)                                  //\n");
    printf("//                                                                                      //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////////////\n\n");

    printf("USAGE:\n\n");
    printf("    > rrem [--help] <filename01.ext> [path/filename02.ext] [path02/filename03.ext]\n");
    printf("           [--output <filename.rres>] [--comp <value>] [--gen-object] [--no-cdir]\n\n");

    printf("OPTIONS:\n\n");
    printf("    -h, --help                      : Show tool version and command line usage help\n\n");

    printf("    <filename01.ext>:<comp>:<crypto>:<force-raw>:<force-id>\n\n");
    printf("                                    : Define input files, one after another, space separated\n");
    printf("                                      with desired configuration parameters for every file, ':' separated.\n");
    
    printf("                                      Supported optional file parameters:\n");
    printf("                                        <comp> : Compression type for the resource.\n");
    printf("                                          Possible values (provided as text):\n");
    printf("                                            COMP_NONE       - No data compression\n");
    printf("                                            COMP_RLE        - RLE (custom) compression\n");
    printf("                                            COMP_DEFLATE    - DEFLATE compression\n");
    printf("                                            COMP_LZ4        - LZ4 compression\n");
    printf("                                            COMP_LZMA2      - LZMA2 compression\n");
    printf("                                            COMP_BZIP2      - BZIP2 compression\n\n");

    printf("                                        <encrypt> : Encryption type for the resource: Possible values\n");
    printf("                                          Possible values (provided as text):\n");
    printf("                                            CRYPTO_NONE     - No data encryption\n");
    printf("                                            CRYPTO_XOR      - XOR (128 bit) encryption\n");
    printf("                                            CRYPTO_AES      - RIJNDAEL (128 bit) encryption (AES)\n");
    printf("                                            CRYPTO_TDES     - Triple DES encryption\n");
    printf("                                            CRYPTO_BLOWFISH - BLOWFISH encryption\n\n");
    
    printf("                                        <force-raw> : Force resource embedding as type RAWD\n");
    printf("                                          Possible values (provided as text):\n");
    printf("                                            AUTO            - Automatically scan type of resource\n");
    printf("                                            FORCE_RAW       - Force resource to be processed as RAW\n\n");
    
    printf("                                        <force-id> : Force resource ID provided (32bit integer)\n\n");

    printf("    -o, --output <filename.rres>    : Define output file.\n");
    printf("                                      Supported extensions: .rres, .h\n");
    printf("    -c, --comp <value>              : Define general data compression method, to be used in case\n");
    printf("                                      not specified on every file individually.\n");
    printf("    --no-cdir                       : Avoid central directory resource generation at the end.\n");

    printf("\nEXAMPLES:\n\n");
    printf("    > rrem image01.png image02.jpg mysound.wav\n");
    printf("        Create 'data.rres' and 'data.h' including those 3 files,\n");
    printf("        uses DEFLATE compression for pixel/wave data.\n\n");

    printf("    > rrem --comp COMP_NONE --no-cdir image01.png sound.wav info.txt\n");
    printf("        Create 'data.rres' and 'data.h' including those 3 files,\n");
    printf("        uses NO compression for pixel/wave/text data and avoids\n");
    printf("        creating a central directory at the end of rres file.\n\n");
    
    printf("    > rrem --output images.rres --comp COMP_DEFLATE //\n");
    printf("        image01.png:COMP_NONE:CRYPTO_NONE:AUTO:3456 //\n"); 
    printf("        image02.bmp:COMP_DEFLATE:CRYPTO_NONE:FORCE_RAW:a22bc8 //\n");
    printf("        image26.bmp\n");
    printf("        Create 'images.rres' and 'images.h' including those 3 files,\n");
    printf("        using custom properties for every resource packaging.\n\n");
   
}

// Process command line input
static void ProcessCommandLine(int argc, char *argv[])
{
    // CLI required variables
    bool showUsageInfo = false;         // Toggle command line usage info

#if defined(COMMAND_LINE_ONLY)
    if (argc == 1) showUsageInfo = true;
#endif

    // Process command line arguments
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            showUsageInfo = true;
        }
        else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0))
        {
            // Check for valid upcoming argument
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                // Check for valid file extension: input
                if (IsFileExtension(argv[i + 1], ".png;.bmp;.tga;.jpg;.gif;.psd;.hdr"))
                {
                    if (!FileExists(argv[i + 1])) printf("WARNING: [%s] Provided image file not found, provide a complete directory path\n", argv[i + 1]);
                    else strcpy(inFileName, argv[i + 1]);    // Read input file
                }
                else printf("WARNING: Input file extension not recognized\n");

                i++;
            }
            else printf("WARNING: No input file provided\n");
        }
        else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0))
        {
            // Check for valid upcoming argument
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (IsFileExtension(argv[i + 1], ".png;.raw;.h;.ktx"))
                {
                    strcpy(outFileName, argv[i + 1]);   // Read output file
                }
                else printf("WARNING: Output file extension not recognized\n");

                i++;
            }
            else printf("WARNING: No output file provided\n");
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--format") == 0))
        {
            // Check for valid upcoming argument
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))  // Check if next argument could be readed
            {
                // TODO: Validate format value

                i++;
            }
            else printf("WARNING: Format parameters provided not valid\n");
        }
    }

    // Process all resources with required configuration
    if (inFileName[0] != '\0')
    {
        // Set a default name for output in case not provided
        if (outFileName[0] == '\0') strcpy(outFileName, TextFormat("%s/output.rres", GetDirectoryPath(inFileName)));

        printf("\n");
        printf("Input files:       %s\n", inFileName);
        // TODO: Input files list with properties
        printf("Output file:       %s\n", outFileName);
        printf("\n");
        
        // TODO: Generate rRES file embedding all resources (check if code object file is required!)

        // Export generated resource
        // SaveRRES(rresData, outFileName);

        // TODO: Unloadall loaded data
    }
    else if (!showUsageInfo) printf("WARNING: Not enough arguments provided.");

    if (showUsageInfo) ShowCommandLineInfo();
}

// WARNING: That's not valid because some files imply multiple resource chunks!
static int GetFileType(const char *fileName)
{
    int type = FILE_RAW;

    if (IsFileExtension(fileName, ".png;.bmp;.tga;.gif;.jpg;.psd;.hdr;.dds;.pkm;.ktx;.pvr;.astc")) type = FILE_IMAGE;
    else if (IsFileExtension(fileName, ".txt;.vs;.fs;.info;.c;.h;.json;.xml")) type = FILE_TEXT;
    else if (IsFileExtension(fileName, ".obj;.iqm;.gltf")) type = FILE_MESH;
    else if (IsFileExtension(fileName, ".wav;.mp3;.ogg;.flac")) type = FILE_AUDIO;
    else if (IsFileExtension(fileName, ".fnt;.ttf;.otf")) type = FILE_FONT;
    //else if (IsFileExtension(fileName, ".mtl")) type = FILE_MATERIAL;

    return type;
}

// Process all required files data and generate .rres
// WARNING: Requires global variables: resources, fileCount
static void GenerateRRES(const char *fileName)//, FileInfo *resources, int count)
{
    #define MAX_CENTRALDIR_ENTRIES  1024
    
    int chunkCounter = 0;
    
    FILE *rresFile = fopen(fileName, "wb");

    if (rresFile == NULL) TRACELOG(LOG_WARNING, "[%s] rRES raylib resource file could not be opened", fileName);
    else
    {
        rresFileHeader header = {
            .id[0] = 'r',
            .id[1] = 'R',
            .id[2] = 'E',
            .id[3] = 'S',
            .version = 100,
            .chunkCount = 0,        // WARNING: Resources chunk count is filled at the end
            .cdOffset = 0,          // WARNING: Central directory offset is filled at the end
        };

        // Write rres file header into file
        fwrite(&header, sizeof(rresFileHeader), 1, rresFile);
        
        rresCentralDir cdir = { 0 };
        cdir.entries = (rresDirEntry *)RL_MALLOC(MAX_CENTRALDIR_ENTRIES*sizeof(rresDirEntry));
        cdir.count = 0;     // Use to count central directory entries
        
        int nextChunkOffset = sizeof(rresFileHeader);
        int cdirSize = 0;

        for (int i = 0; i < fileCount; i++)
        {
            if (resFiles[i].forceRaw) resFiles[i].fileType = FILE_RAW;
            unsigned int resId = (resFiles[i].customId > 0)? resFiles[i].customId : resFiles[i].hashId;
            
            TraceLog(LOG_INFO, "[Id: %X] Processing file [%i]: %s", resId, i, resFiles[i].fileName);
            
            // Save required number of resource chunks depending on file type
            switch (resFiles[i].fileType)
            {
                case FILE_RAW:      // Simple data: Raw, (1) chunk
                {
                    // Read file data
                    unsigned int bytesRead = 0;
                    unsigned char *data = LoadFileData(resFiles[i].fileName, &bytesRead);
                    
                    // Create chunk data buffer: RRES_DATA_RAW
                    unsigned int propsCount = 1;    // props[0]:size
                    unsigned int bufferSize = sizeof(int) + propsCount*sizeof(int) + bytesRead;
                    unsigned char *buffer = RL_CALLOC(bufferSize, 1);
                    ((unsigned int *)buffer)[0] = propsCount;
                    ((unsigned int *)buffer)[1] = bytesRead;
                    memcpy(buffer + sizeof(int) + propsCount*sizeof(int), data, bytesRead);

                    // Define data chunk info header
                    rresInfoHeader info = {
                        .type[0] = 'R',
                        .type[1] = 'A',
                        .type[2] = 'W',
                        .type[3] = 'D',
                        .id = resId,                // Resource chunk identifier
                        .compType = (unsigned char)resFiles[i].compType,    // Data compression type
                        .crypType = (unsigned char)resFiles[i].crypType,    // Data encription type
                        .flags = 0,                 // Data flags (if required)
                        .compSize = bufferSize,     // Data compressed size
                        .uncompSize = bufferSize,   // Data uncompressed size
                        .nextOffset = 0,            // Next resource chunk offset (if required)
                        .reserved = 0,
                        .crc32 = 0,                 // Data chunk CRC32 (propsCount + props[] + data)
                    };
                    
                    //------------------------------------------------------------------------------------------------------
                    // Process data buffer
                    unsigned char *compData = NULL;
                    if (info.compType == RRES_COMP_NONE) compData = buffer;
                    else if (info.compType == RRES_COMP_DEFLATE) compData = CompressData(buffer, info.uncompSize, &info.compSize);
                    
                    // Update data chunk info
                    info.crc32 = rresComputeCRC32(compData, info.compSize);
                    
                    // Write resource info and data
                    fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);
                    fwrite(compData, info.compSize, 1, rresFile);
                    
                    // Free all loaded resources
                    if (info.compType > RRES_COMP_NONE) RL_FREE(compData);
                    //------------------------------------------------------------------------------------------------------
                    RL_FREE(buffer);
                    UnloadFileData(data);

                    // Register central directory entry
                    cdir.entries[cdir.count].id = resId;
                    cdir.entries[cdir.count].offset = nextChunkOffset;
                    cdir.entries[cdir.count].fileNameLen = strlen(resFiles[i].fileName) + 1;  // EOL:'\0'
                    strcpy(cdir.entries[cdir.count].fileName, resFiles[i].fileName);
                    cdirSize += (12 + cdir.entries[cdir.count].fileNameLen);    // 12 bytes: (id + offset + fileNameLen)
                    
                    nextChunkOffset += (sizeof(rresInfoHeader) + info.compSize);
                    cdir.count++;
                    chunkCounter++;
            
                } break;
                case FILE_TEXT:     // Simple data: Text, (1) chunk
                {
                    // Read file data
                    char *text = LoadFileText(resFiles[i].fileName);
                    unsigned int textLen = strlen(text);
                    
                    // Create chunk data buffer: RRES_DATA_RAW
                    unsigned int propsCount = 2;    // props[0]:size, props[1]:cultureCode
                    unsigned int bufferSize = sizeof(int) + propsCount*sizeof(int) + textLen;
                    unsigned char *buffer = RL_CALLOC(bufferSize, 1);
                    ((unsigned int *)buffer)[0] = propsCount;
                    ((unsigned int *)buffer)[1] = textLen;
                    ((unsigned int *)buffer)[2] = 0x0409;    // en-US: English - United States
                    memcpy(buffer + sizeof(int) + propsCount*sizeof(int), text, textLen);

                    // Define data chunk info header
                    rresInfoHeader info = {
                        .type[0] = 'T',
                        .type[1] = 'E',
                        .type[2] = 'X',
                        .type[3] = 'T',
                        .id = resId,                // Resource chunk identifier
                        .compType = (unsigned char)resFiles[i].compType,    // Data compression type
                        .crypType = (unsigned char)resFiles[i].crypType,    // Data encription type
                        .flags = 0,                 // Data flags (if required)
                        .compSize = bufferSize,     // Data compressed size
                        .uncompSize = bufferSize,   // Data uncompressed size
                        .nextOffset = 0,            // Next resource chunk offset (if required)
                        .reserved = 0,
                        .crc32 = 0,                 // Data chunk CRC32 (propsCount + props[] + data)
                    };
                    
                    //------------------------------------------------------------------------------------------------------
                    // Process data buffer
                    unsigned char *compData = NULL;
                    if (info.compType == RRES_COMP_NONE) compData = buffer;
                    else if (info.compType == RRES_COMP_DEFLATE) compData = CompressData(buffer, info.uncompSize, &info.compSize);
                    
                    // Update data chunk info
                    info.crc32 = rresComputeCRC32(compData, info.compSize);
                    
                    // Write resource info and data
                    fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);
                    fwrite(compData, info.compSize, 1, rresFile);
                    
                    // Free all loaded resources
                    if (info.compType > RRES_COMP_NONE) RL_FREE(compData);
                    //------------------------------------------------------------------------------------------------------
                    RL_FREE(buffer);
                    UnloadFileText(text);

                    // Register central directory entry
                    cdir.entries[cdir.count].id = resId;
                    cdir.entries[cdir.count].offset = nextChunkOffset;
                    cdir.entries[cdir.count].fileNameLen = strlen(resFiles[i].fileName) + 1;  // EOL:'\0'
                    strcpy(cdir.entries[cdir.count].fileName, resFiles[i].fileName);
                    cdirSize += (12 + cdir.entries[cdir.count].fileNameLen);    // 12 bytes: (id + offset + fileNameLen)
                    
                    nextChunkOffset += (sizeof(rresInfoHeader) + info.compSize);
                    cdir.count++;
                    chunkCounter++;
                    
                } break;
                case FILE_IMAGE:    // Simple data: Image, (1) chunk
                {
                    // Read file data
                    Image image = LoadImage(resFiles[i].fileName);
                    unsigned int imDataSize = GetPixelDataSize(image.width, image.height, image.format);
                    
                    // Create chunk data buffer: RRES_DATA_IMAGE
                    unsigned int propsCount = 4;    // props[0]:width, props[1]:height, props[2]:mipmaps, props[3]:rresPixelFormat
                    unsigned int bufferSize = sizeof(int) + propsCount*sizeof(int) + imDataSize;
                    unsigned char *buffer = RL_CALLOC(bufferSize, 1);
                    ((unsigned int *)buffer)[0] = propsCount;
                    ((unsigned int *)buffer)[1] = image.width;
                    ((unsigned int *)buffer)[2] = image.height;
                    ((unsigned int *)buffer)[3] = image.format;
                    ((unsigned int *)buffer)[4] = image.mipmaps;
                    memcpy(buffer + sizeof(int) + propsCount*sizeof(int), image.data, imDataSize);

                    // Define data chunk info header
                    rresInfoHeader info = {
                        .type[0] = 'I',
                        .type[1] = 'M',
                        .type[2] = 'G',
                        .type[3] = 'E',
                        .id = resId,                // Resource chunk identifier
                        .compType = (unsigned char)resFiles[i].compType,    // Data compression type
                        .crypType = (unsigned char)resFiles[i].crypType,    // Data encription type
                        .flags = 0,                 // Data flags (if required)
                        .compSize = bufferSize,     // Data compressed size
                        .uncompSize = bufferSize,   // Data uncompressed size
                        .nextOffset = 0,            // Next resource chunk offset (if required)
                        .reserved = 0,
                        .crc32 = 0,                 // Data chunk CRC32 (propsCount + props[] + data)
                    };
                    
                    
                    //rresWriteResource(FILE *file, rresInfoHeader *info, unsigned char *buffer, unsigned int bufferSize);
                    //------------------------------------------------------------------------------------------------------
                    // Process data buffer
                    unsigned char *compData = NULL;
                    if (info.compType == RRES_COMP_NONE) compData = buffer;
                    else if (info.compType == RRES_COMP_DEFLATE) compData = CompressData(buffer, info.uncompSize, &info.compSize);
                    
                    // Update data chunk info
                    info.crc32 = rresComputeCRC32(compData, info.compSize);
                    
                    // Write resource info and data
                    fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);     // Use a memory buffer?
                    fwrite(compData, info.compSize, 1, rresFile);
                    
                    // Free all loaded resources
                    if (info.compType > RRES_COMP_NONE) RL_FREE(compData);
                    //------------------------------------------------------------------------------------------------------
                    RL_FREE(buffer);
                    UnloadImage(image);
                    
                    // Register central directory entry
                    cdir.entries[cdir.count].id = resId;
                    cdir.entries[cdir.count].offset = nextChunkOffset;
                    cdir.entries[cdir.count].fileNameLen = strlen(resFiles[i].fileName) + 1;  // EOL:'\0'
                    strcpy(cdir.entries[cdir.count].fileName, resFiles[i].fileName);
                    cdirSize += (12 + cdir.entries[cdir.count].fileNameLen);    // 12 bytes: (id + offset + fileNameLen)
                    
                    nextChunkOffset += (sizeof(rresInfoHeader) + info.compSize);
                    cdir.count++;
                    chunkCounter++;
                    
                } break;
                case FILE_AUDIO:    // Simple data: Audio, (1) chunk
                {
                    // Read file data
                    Wave wave = LoadWave(resFiles[i].fileName);
                    unsigned int waveDataSize = wave.sampleCount*wave.sampleSize/8;
                    
                    // Create chunk data buffer: RRES_DATA_WAVE
                    unsigned int propsCount = 4;    // props[0]:sampleCount, props[1]:sampleRate, props[2]:sampleSize, props[3]:channels
                    unsigned int bufferSize = sizeof(int) + propsCount*sizeof(int) + waveDataSize;
                    unsigned char *buffer = RL_CALLOC(bufferSize, 1);
                    ((unsigned int *)buffer)[0] = propsCount;
                    ((unsigned int *)buffer)[1] = wave.sampleCount;
                    ((unsigned int *)buffer)[2] = wave.sampleRate;
                    ((unsigned int *)buffer)[3] = wave.sampleSize;
                    ((unsigned int *)buffer)[4] = wave.channels;
                    memcpy(buffer + sizeof(int) + propsCount*sizeof(int), wave.data, waveDataSize);

                    // Define data chunk info header
                    rresInfoHeader info = {
                        .type[0] = 'W',
                        .type[1] = 'A',
                        .type[2] = 'V',
                        .type[3] = 'E',
                        .id = resId,                // Resource chunk identifier
                        .compType = (unsigned char)resFiles[i].compType,    // Data compression type
                        .crypType = (unsigned char)resFiles[i].crypType,    // Data encription type
                        .flags = 0,                 // Data flags (if required)
                        .compSize = bufferSize,     // Data compressed size
                        .uncompSize = bufferSize,   // Data uncompressed size
                        .nextOffset = 0,            // Next resource chunk offset (if required)
                        .reserved = 0,
                        .crc32 = 0,                 // Data chunk CRC32 (propsCount + props[] + data)
                    };
                    
                    //------------------------------------------------------------------------------------------------------
                    // Process data buffer
                    unsigned char *compData = NULL;
                    if (info.compType == RRES_COMP_NONE) compData = buffer;
                    else if (info.compType == RRES_COMP_DEFLATE) compData = CompressData(buffer, info.uncompSize, &info.compSize);
                    
                    // Update data chunk info
                    info.crc32 = rresComputeCRC32(compData, info.compSize);
                    
                    // Write resource info and data
                    fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);
                    fwrite(compData, info.compSize, 1, rresFile);
                    
                    // Free all loaded resources
                    if (info.compType > RRES_COMP_NONE) RL_FREE(compData);
                    //------------------------------------------------------------------------------------------------------
                    RL_FREE(buffer);
                    UnloadWave(wave);
                    
                    // Register central directory entry
                    cdir.entries[cdir.count].id = resId;
                    cdir.entries[cdir.count].offset = nextChunkOffset;
                    cdir.entries[cdir.count].fileNameLen = strlen(resFiles[i].fileName) + 1;  // EOL:'\0'
                    strcpy(cdir.entries[cdir.count].fileName, resFiles[i].fileName);
                    cdirSize += (12 + cdir.entries[cdir.count].fileNameLen);    // 12 bytes: (id + offset + fileNameLen)
                                       
                    nextChunkOffset += (sizeof(rresInfoHeader) + info.compSize);
                    cdir.count++;
                    chunkCounter++;
                    
                } break;
                case FILE_FONT:     // Complex data: Font, (2) chunks
                {
                    
                    
                    cdir.count++;
                    chunkCounter += 2;
                    
                } break;
                case FILE_MESH:     // Complex data: Mesh, (n) chunks
                {
                    
                    cdir.count++;
                    chunkCounter += 6;
                    
                } break;
                default: break;
            }
        }
               
        if (saveCentralDir)
        {           
            // Create chunk data buffer: RRES_DATA_DIRECTORY
            unsigned int propsCount = 1;        // props[0]:entryCount
            unsigned int bufferSize = sizeof(int) + propsCount*sizeof(int) + cdirSize;
            unsigned char *buffer = RL_CALLOC(bufferSize, 1);
            ((unsigned int *)buffer)[0] = propsCount;
            ((unsigned int *)buffer)[1] = cdir.count;
            for (unsigned int i = 0, offset = 0; i < cdir.count; i++)
            {
                TraceLog(LOG_INFO, "CDIR: [%08X] Entry (0x%x): %s (len: %i)", cdir.entries[i].id, cdir.entries[i].offset, cdir.entries[i].fileName, cdir.entries[i].fileNameLen);

                memcpy(buffer + offset, &cdir.entries[i].id, sizeof(int));
                memcpy(buffer + offset + 4, &cdir.entries[i].offset, sizeof(int));
                memcpy(buffer + offset + 8, &cdir.entries[i].fileNameLen, sizeof(int));
                memcpy(buffer + offset + 12, cdir.entries[i].fileName, cdir.entries[i].fileNameLen);
                
                offset += (12 + cdir.entries[i].fileNameLen);
            }
            
            // Define data chunk info header 
            rresInfoHeader info = {
                .type[0] = 'C',
                .type[1] = 'D',
                .type[2] = 'I',
                .type[3] = 'R',
                .id = 0,                        // Resource chunk identifier
                .compType = RRES_COMP_NONE,     // Data compression type
                .crypType = RRES_CRYPTO_NONE,   // Data encription type
                .flags = 0,                     // Data flags (if required)
                .compSize = cdirSize,           // Data compressed size
                .uncompSize = cdirSize,         // Data uncompressed size
                .nextOffset = 0,                // Next resource chunk offset (if required)
                .reserved = 0,
                .crc32 = 0,                     // Data chunk CRC32 (propsCount + props[] + data)
            };

            //------------------------------------------------------------------------------------------------------
            // Process data buffer
            unsigned char *compData = NULL;
            if (info.compType == RRES_COMP_NONE) compData = buffer;
            else if (info.compType == RRES_COMP_DEFLATE) compData = CompressData(buffer, info.uncompSize, &info.compSize);
            
            // Update data chunk info
            info.crc32 = rresComputeCRC32(compData, info.compSize);
            
            // Write resource info and data
            fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);
            fwrite(compData, info.compSize, 1, rresFile);
            
            // Free all loaded resources
            if (info.compType > RRES_COMP_NONE) RL_FREE(compData);
            //------------------------------------------------------------------------------------------------------
            RL_FREE(buffer);
        }
       
        RL_FREE(cdir.entries);
        
        // Update rres file header
        header.chunkCount = (unsigned short)chunkCounter;
        header.cdOffset = (unsigned int)nextChunkOffset - sizeof(rresFileHeader);
        fseek(rresFile, 6, SEEK_SET);
        fwrite(&header.chunkCount, sizeof(unsigned short), 1, rresFile);
        fwrite(&header.cdOffset, sizeof(unsigned int), 1, rresFile);
        
        TraceLog(LOG_WARNING, "CDIR: Offset: %08x", header.cdOffset);
    }
    
    fclose(rresFile);
}

//--------------------------------------------------------------------
// Auxiliar functions (utilities)
//---------------------------------------------------------------------

// Compute chunk hash id from filename
unsigned int ComputeHashId(const char *fileName)
{
    unsigned int hash = 0;
    unsigned int length = strlen(fileName);
    
    for (unsigned int i = 0; i < length; i++) hash = hash*31 + (unsigned int)fileName[i];
    
    return hash;
}

// Swap 16 bit data
static unsigned short Swap16bit(unsigned short us)
{
    return (unsigned short)(((us & 0xFF00) >> 8) |
                            ((us & 0x00FF) << 8));
}

// Swap 32 bit data
static unsigned long Swap32bit(unsigned long ul)
{
    return (unsigned long)(((ul & 0xFF000000) >> 24) |
                           ((ul & 0x00FF0000) >>  8) |
                           ((ul & 0x0000FF00) <<  8) |
                           ((ul & 0x000000FF) << 24));
}

// Converts file binary data into byte array code (.h)
static void SaveFileAsCode(const char *fileName, const char *data, int dataSize)
{
	FILE *codeFile;

    codeFile = fopen(fileName, "wt");

    fprintf(codeFile, "const int dataSize = %i;", dataSize);
	fprintf(codeFile, "const unsigned char %s[%i] = {\n    ", fileName, dataSize);
	
	int blCounter = 0;		// Break line counter
	
	for (int i = 0; i < dataSize; i ++)
	{
		blCounter++;

        fprintf(codeFile, "0x%.2x, ", data[i]);
		
        if (blCounter >= 24)
		{
			fprintf(codeFile, "\n    ");
			blCounter = 0;
		}
	}

    fprintf(codeFile, " };\n");

	fclose(codeFile);
}

// Check if text string is codified as UTF-8
static bool IsUtf8(const char *string)
{
    if (!string) return 0;

    const unsigned char *bytes = (const unsigned char *)string;

    while (*bytes)
    {
        if( (// ASCII
             // use bytes[0] <= 0x7F to allow ASCII control characters
                bytes[0] == 0x09 ||
                bytes[0] == 0x0A ||
                bytes[0] == 0x0D ||
                (0x20 <= bytes[0] && bytes[0] <= 0x7E)
            )
        ) {
            bytes += 1;
            continue;
        }

        if( (// non-overlong 2-byte
                (0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF)
            )
        ) {
            bytes += 2;
            continue;
        }

        if( (// excluding overlongs
                bytes[0] == 0xE0 &&
                (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            ) ||
            (// straight 3-byte
                ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
                    bytes[0] == 0xEE ||
                    bytes[0] == 0xEF) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            ) ||
            (// excluding surrogates
                bytes[0] == 0xED &&
                (0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            )
        ) {
            bytes += 3;
            continue;
        }

        if( (// planes 1-3
                bytes[0] == 0xF0 &&
                (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            ) ||
            (// planes 4-15
                (0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            ) ||
            (// plane 16
                bytes[0] == 0xF4 &&
                (0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            )
        ) {
            bytes += 4;
            continue;
        }

        return 0;
    }

    return 1;
}
