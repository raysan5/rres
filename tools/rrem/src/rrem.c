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

#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"           // GUI: File Dialogs

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

// Loaded resource file required info
typedef struct {
    int hashId;         // Id no customId provided uses hashId
    int customId;
    int type;
    int compType;
    int crypType;
    bool forceRaw;
    
    // Central directory data
    int fileNameLength;
    char *fileName;
} FileInfo;

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
static const char *toolName = "rREM";
static const char *toolVersion = "1.0";
static const char *toolDescription = "A simple and easy-to-use rres resources packer";

static FileInfo resources[MAX_RESOURCES] = { 0 };
static unsigned int fileCount = 0;
static unsigned int prevResCount = 0;

static bool saveCentralDir = true;

// Move all variables here as globals for web compilation
static char inFileName[512] = { 0 };       // Input file name (required in case of drag & drop over executable)
static char outFileName[512] = { 0 };      // Output file name (required for file save/export)

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input

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
    FreeConsole();
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
                    // TODO: ScanDirectoryTreeFiles(droppedFiles[i]);
                }
                else 
                {
                    strcpy(resources[fileCount].fileName, droppedFiles[i]);
                    
                    fileCount++;
                    if (fileCount >= MAX_RESOURCES) break;
                }
            }

            ClearDroppedFiles();
            
            // Review new files types added to list
            if (fileCount > prevResCount)
            {
                for (int i = prevResCount; i < fileCount; i++)
                {
                    if (IsFileExtension(resources[i].fileName, ".png;.bmp;.tga;.gif;.jpg;.psd;.hdr;.dds;.pkm;.ktx;.pvr;.astc")) resources[i].type = RRES_DATA_IMAGE;
                    else if (IsFileExtension(resources[i].fileName, ".txt;.vs;.fs;.info;.c;.h;.json;.xml")) resources[i].type = RRES_DATA_TEXT;
                    else if (IsFileExtension(resources[i].fileName, ".obj;.iqm;.gltf")) resources[i].type = RRES_DATA_MESH;
                    else if (IsFileExtension(resources[i].fileName, ".wav")) resources[i].type = RRES_DATA_WAVE;
                    else if (IsFileExtension(resources[i].fileName, ".fnt")) resources[i].type = RRES_DATA_FONT;
                    else if (IsFileExtension(resources[i].fileName, ".mtl")) resources[i].type = RRES_DATA_MATERIAL;
                    else resources[i].type = RRES_DATA_RAW;
                }
            }
            
            prevResCount = fileCount;
        }

        // TODO: Implement files scrolling with mouse wheel
        // TODO: Implement files focus on mouse-over and file select/unselect on click
        // TODO: Implement file configuration (compression/encryption) --> use raygui

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

                    DrawText(resources[i].fileName, 120, 100 + 40*i, 10, GRAY);
                }

                DrawText("Drop new files...", 100, 110 + 40*fileCount, 20, DARKGRAY);
            }
            
            DrawText(TextFormat("%02i", fileCount), 10, 10, 20, MAROON);
            
            if (GuiButton((Rectangle){ 10, GetScreenHeight() - 50, 160, 40 }, "Generate RRES"))
            {
                GenerateRRES("test.rres");
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
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
    
    int compressionType = 0;
    
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

static int GetFileType(const char *fileName)
{
    int type = RRES_DATA_RAW;

    if (IsFileExtension(fileName, ".png;.bmp;.tga;.gif;.jpg;.psd;.hdr;.dds;.pkm;.ktx;.pvr;.astc")) type = RRES_DATA_IMAGE;
    else if (IsFileExtension(fileName, ".txt;.vs;.fs;.info;.c;.h;.json;.xml")) type = RRES_DATA_TEXT;
    else if (IsFileExtension(fileName, ".obj;.iqm;.gltf")) type = RRES_DATA_MESH;
    else if (IsFileExtension(fileName, ".wav")) type = RRES_DATA_WAVE;
    else if (IsFileExtension(fileName, ".fnt")) type = RRES_DATA_FONT;
    else if (IsFileExtension(fileName, ".mtl")) type = RRES_DATA_MATERIAL;

    return type;
}

// Process all required files data and generate .rres
// WARNING: Requires global variables: resources, fileCount
static void GenerateRRES(const char *fileName)//, FileInfo *resources, int count)
{
    int chunksCounter = 0;
    FILE *rresFile = fopen(fileName, "wb");

    if (rresFile == NULL) TRACELOG(LOG_WARNING, "[%s] rRES raylib resource file could not be opened", fileName);
    else
    {
        rresFileHeader header = { 0 };  // WARNING: File Header not exposed!
        header.id[0] = 'r';
        header.id[1] = 'R';
        header.id[2] = 'E';
        header.id[3] = 'S';
        header.version = 100;
        header.resCount = fileCount;    // WARNING: Resources chunk count is not the same as file count!
        header.cdOffset = 0;            // WARNING: This value is filled at the end (if required)

        // Write rres file header into file
        fwrite(&header, sizeof(rresFileHeader), 1, rresFile);
        
        rresCentralDir cdir = { 0 };
        cdir.count = header.resCount;
        cdir.entries = (rresDirEntry *)RRES_MALLOC(cdir.count*sizeof(rresDirEntry));
        int resOffset = sizeof(rresFileHeader);
        int cdirSize = 0;

        for (int i = 0; i < header.resCount; i++)
        {
            rresInfoHeader info = { 0 };
            
            switch(resources[i].type)
            {
                case RRES_DATA_RAW:
                {
                    // Save RAW file
                } break;
                case RRES_DATA_TEXT:
                {

                } break;
                case RRES_DATA_IMAGE:
                {
                    Image image = LoadImage(resources[i].fileName);
                    
                    rresDataChunk *chunk = (rresDataChunk *)RRES_CALLOC(sizeof(rresDataChunk), 1);
                    chunk->propsCount = 4;
                    chunk->props = (int *)RRES_MALLOC(chunk->propsCount*sizeof(int));
                    chunk->props[0] = image.width;
                    chunk->props[1] = image.height;
                    chunk->props[2] = image.format;
                    chunk->props[3] = image.mipmaps;
                    chunk->data = image.data;
                    
                    strncpy(info.type, "IMGE", 4);
                    info.id = resources[i].hashId;    // TODO: rresComputeHashId();
                    info.compType = resources[i].compType;
                    info.crypType = resources[i].crypType;
                    info.flags = 0;
                    info.compSize = 0;  // Filled on compression
                    info.uncompSize = GetPixelDataSize(image.width, image.height, image.format) + chunk->propsCount*sizeof(int);
                    
                    // NOTE: Depending on the type of data to package, we will need a next chunk offset 
                    info.nextOffset = 0;

                    unsigned char *compData = NULL;
                    
                    // NOTE: It is better to compress before encrypting but any proven block cipher will reduce the data to 
                    // a pseudo-random sequence of bytes that will typically yield little to no compression gain at all, so,
                    // in most cases it's better to just encrypt the uncompressed data and be done with it.
                    
                    if (info.compSize == RRES_COMP_NONE) compData = (((unsigned char *)chunk) + 4);
                    else if (info.compSize == RRES_COMP_DEFLATE)
                    {
                        compData = CompressData(((unsigned char *)chunk) + 4, info.uncompSize, &info.compSize);
                    }
                    
                    info.crc32 = ComputeCRC32(compData, info.compSize);
                    
                    // Write resource info and data
                    fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);
                    fwrite(compData, info.compSize, 1, rresFile);
                    
                    // Free all loaded resources
                    if (info.compSize == RRES_COMP_DEFLATE) RRES_FREE(compData);
                    RRES_FREE(chunk->props);
                    RRES_FREE(chunk->data);      // Unloads image.data
                    RRES_FREE(chunk);
                } break;
                case RRES_DATA_WAVE:
                {

                } break;
                case RRES_DATA_FONT:
                {
                    
                } break;
                case RRES_DATA_MATERIAL:
                {
                    
                } break;
                case RRES_DATA_MESH:
                {
                    
                } break;
                default: break;
            }
            
            cdir.entries[i].id = resources[i].hashId;   // Resource unique ID
            
            // Resource offset in file
            resOffset += (sizeof(rresInfoHeader) + info.compSize);
            cdir.entries[i].offset = resOffset;
            
            // Resource fileName info ('\0' terminated)
            cdir.entries[i].fileNameLen = strlen(resources[i].fileName) + 1;
            cdir.entries[i].fileName = (char *)RRES_CALLOC(cdir.entries[i].fileNameLen + 1, 1);
            strcpy(cdir.entries[i].fileName, resources[i].fileName);
            cdirSize += (12 + cdir.entries[i].fileNameLen + 1);
        }
        
        if (saveCentralDir)
        {
            // Write rres central directory info
            rresDataChunk *chunk = (rresDataChunk *)RRES_CALLOC(sizeof(rresDataChunk), 1);
            chunk->propsCount = 1;
            chunk->props = (int *)RRES_MALLOC(chunk->propsCount*sizeof(int));
            chunk->props[0] = cdir.count;
            chunk->data = cdir.entries;
            
            rresInfoHeader info = { 0 };
            strncpy(info.type, "CDIR", 4);
            info.id = 0;        
            info.compType = 0;
            info.crypType = 0;
            info.flags = 0;
            info.compSize = cdirSize;
            info.uncompSize = cdirSize;
            info.nextOffset = 0;

            unsigned char *compData = NULL;

            if (info.compSize == RRES_COMP_NONE) compData = (((unsigned char *)chunk) + 4);
            
            info.crc32 = ComputeCRC32(compData, info.compSize);
            
            // Write resource info and data
            fwrite(&info, sizeof(rresInfoHeader), 1, rresFile);
            fwrite(compData, info.compSize, 1, rresFile);
            
            // Free all loaded resources
            RRES_FREE(chunk->props);
            RRES_FREE(chunk);
            
            for (int i = 0; i < header.resCount; i++) RRES_FREE(cdir.entries[i].fileName);
            RRES_FREE(cdir.entries);
        }
    }
    
    fclose(rresFile);
}

//--------------------------------------------------------------------
// Auxiliar functions (utilities)
//---------------------------------------------------------------------

// Convert int value into 4-bytes array (char buffer)
static unsigned char *IntToChar(int myint)
{
    unsigned char mychars[4];

    mychars[3] = (myint & (0xFF));
    mychars[2] = ((myint >> 8) & 0xFF);
    mychars[1] = ((myint >> 16) & 0xFF);
    mychars[0] = ((myint >> 24) & 0xFF);

/*
    unsigned int final = 0;
    final |= ( mychars[0] << 24 );
    final |= ( mychars[1] << 16 );
    final |= ( mychars[2] <<  8 );
    final |= ( mychars[3]       );

    int num = (chars[0] << 24) + (chars[1] << 16) + (chars[2] << 8) + chars[3];

    int num = 0;
    for (int i = 0; i != 4; ++i) {
        num += chars[i] << (24 - i * 8);    // |= could have also been used
    }
*/
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
