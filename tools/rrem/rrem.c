/**********************************************************************************************
*
*   rREM v0.5 - A simple and easy to use raylib resource embedder
*
*   DESCRIPTION:
*       Tool to embbed resources (images, text, sounds...) into a file; possible uses:
*         - Embedding into a resource file (.rres)
*         - Directly embedding in executable program (.exe)
*
*   DEPENDENCIES:
*       raylib 2.0      - Windowing/input management and drawing.
*       raygui 2.0      - IMGUI controls (based on raylib).
*       miniz           - DEFLATE compression/decompression library (zlib-style)
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
*   Copyright (c) 2014-2017 Ramon Santamaria (@raysan5)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>             // String management functions: strlen(), strrchr(), strcmp()

#include "raylib.h"             // Required for data loading and management

#define RAYGUI_IMPLEMENTATION
#include "external/raygui.h"    // Required for: IMGUI implementation

//#define RRES_IMPLEMENTATION
//#include "rres.h"               // Required for: rRES management --> Not yet available...

//#include "external/tinyfiledialogs.h"   // Required for: Open/Save file dialogs
#include "external/miniz.h"             // Required for: DEFLATE compression/decompression

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define ENABLE_PRO_FEATURES             // Enable PRO version features

#define TOOL_VERSION_TEXT     "0.5"     // Tool version string

#define MAX_PATH_LENGTH        256      // Maximum length for resources file paths

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Loaded resource file required info
// TODO: Use rres predefined structs, some info is not exposed...
typedef struct {
    unsigned short resId;
    unsigned char fileName[256];
    unsigned int fileType;
    unsigned int compType;
    unsigned int crypType;
    unsigned int paramsCount;
    unsigned int dataSize;
    int *params;
    void *data;
    //RRES resource;
} ResourceInfo;

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static void ShowUsageInfo(void);    // Show command line usage info

static int GetRRESFileType(const char *ext);
//static void GenRRESHeaderFile(const char *rresHeaderName, RRES *resources, int resCount);
static void GenRRESObjectFile(const char *rresFileName);

// TODO: Compression/Decompression functions and Encryption/Decription functions should be provided by external lib
//static unsigned char *CompressData(const unsigned char *data, unsigned long uncompSize, unsigned long *outCompSize);
//static unsigned char *DecompressData(const unsigned char *data, unsigned long compSize, int uncompSize);


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // NOTE: We DON'T need the current working directory! We can use the rrem.exe folder
    // It's easier, just get the folder from argv[0]

    char currentPath[MAX_PATH_LENGTH];
    strcpy(currentPath, GetWorkingDirectory());

    ResourceInfo resources[256];       // RRES_MAX_RESOURCES
    unsigned int resCount = 0;
    
    // Command-line usage mode
    //--------------------------------------------------------------------------------------
    if (argc > 1)
    {
        // TODO: Parse arguments and fill resources[] array
        int compressionType = 0;
        
        // Before starting, process all arguments for options...
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if ((strcmp(argv[i], "-v")==0) || (strcmp(argv[i], "--version")==0))
                {
                    // Print version information and exit immediately
                    printf("%s", TOOL_VERSION_TEXT);
                    exit(0);
                }
                else if ((strcmp(argv[i], "-h")==0) || (strcmp(argv[i], "--help")==0))
                {
                    // Print help and exit immediately
                    //printf("%s", usageHelp);
                    exit(0);
                }
                else if (strcmp(argv[i], "-o")==0)
                {
                    // Generate an embeddable object (.o file)
                    //genObjRes = true;
                    //firstFileArgPos++;
                    
                    printf("Generating an embeddable object!\n");
                }
                else if (argv[i][1] == 'c')
                {
                    // Specified compression algorithm for data (default: DEFLATE)
                    switch (argv[i][2])
                    {
                        case '0':
                        {
                            compressionType = 0;
                            printf("Data compression algorithm selected: NO COMPRESSION\n");
                        } break;
                        case '1':
                        {
                            compressionType = 1;
                            printf("Data compression algorithm selected: RLE (CUSTOM)\n");
                        } break;
                        case '2':
                        {
                            compressionType = 2;
                            printf("Data compression algorithm selected: DEFLATE\n");
                        } break;
                        case '3':
                        {
                            compressionType = 2;
                            printf("Data compression algorithm selected: LZMA (NOT IMPLEMENTED YET)\n");
                        } break;
                        case '4':
                        {
                            compressionType = 2;
                            printf("Data compression algorithm selected: BZ2 (NOT IMPLEMENTED YET)\n");
                        } break;
                        default: break;
                    }
                    
                    //firstFileArgPos++;
                }
            }
        }
        
        // TODO: Process all resources with required configuration
        
        // TODO: Generate rRES file embedding all resources (check if code object file is required!)
        
        return 0;
    }

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 720;
    const int screenHeight = 640;
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, FormatText("rREM v%s - A simple and easy-to-use raylib resource embedded", TOOL_VERSION_TEXT));

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
                // TODO: Probably we need to use struct RRES and struct RRESChunk
                
                resources[resCount + i].resId = 0;          // TODO: GenerateRRESId()
                strcpy(resources[resCount + i].fileName, droppedFiles[i]);
                resources[resCount + i].fileType = GetRRESFileType(GetExtension(droppedFiles[i]));
                resources[resCount + i].compType = 0;       // Default compression: NONE
                resources[resCount + i].crypType = 0;       // Default encryption: NONE
                
                // TODO: Read file data (depends on fileType)
                
                // TODO: Support multiple chunks resources!
                
                resources[resCount + i].paramsCount = 4;    // TODO: Depends on fileType
                resources[resCount + i].dataSize = 0;       // TODO: We get it when reading file...
                
                resources[resCount + i].params = (int *)malloc(sizeof(int)*resources[resCount + i].paramsCount);
                
                // TODO: Fill data parameters (depends on fileType)
                
                resources[resCount + i].data = 0;           // TODO: Point to resource chunk data
            }
            
            resCount += dropsCount;
            dropsCount = 0;

            ClearDroppedFiles();
        }
        
        // TODO: Implement files scrolling with mouse wheel
        
        // TODO: Implement files focus on mouse-over and file select/unselect on click

        // TODO: Implement file configuration (compression/encryption) --> use raygui
        
        // TODO: On SaveRRESFile(), process all resources data (compress/encrypt) --> second thread with progress bar?
        
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            if (resCount == 0) DrawText("Drop your files to this window!", 100, 40, 20, DARKGRAY);
            else
            {
                DrawText("Dropped files:", 100, 40, 20, DARKGRAY);
                
                for (int i = 0; i < resCount; i++)
                {
                    if (i%2 == 0) DrawRectangle(0, 85 + 40*i, GetScreenWidth(), 40, Fade(LIGHTGRAY, 0.5f));
                    else DrawRectangle(0, 85 + 40*i, GetScreenWidth(), 40, Fade(LIGHTGRAY, 0.3f));
                    
                    DrawText(resources[i].fileName, 120, 100 + 40*i, 10, GRAY);
                }
                
                DrawText("Drop new files...", 100, 110 + 40*resCount, 20, DARKGRAY);
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//--------------------------------------------------------------------------------------------

// Show command line usage info
static void ShowUsageInfo(void)
{
    printf("\n//////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                              //\n");
    printf("// rREM v%s - A simple and easy-to-use raylib resource embedder                //\n", TOOL_VERSION_TEXT);
    printf("// powered by raylib v2.1 (www.raylib.com) and raygui v2.1                      //\n");
    printf("// more info and bugs-report: github.com/raysan5/rres                           //\n");
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2014-2018 Ramon Santamaria (@raysan5)                          //\n");
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");
    
#if defined(ENABLE_PRO_FEATURES)

// rrem creates a .rres resource with embedded files and a .h header to access embedded data.
    
*
*   USAGE: rrem [-v] [-h] [-o] [-cx] [-n <output_name>] <input_file> [<another_input_file>]
*
*          -v                       Print version information and exit immediately
*          -h                       Print usage help and exit immediately
*          -i                       Activates rayGUI custom user interface
*          -n <output_file_name>    Name for the output file (default: data.rres) (NOT IMPLEMENTED)
*          -a <rres_file_name>      Append data to an existing .rres file (NOT IMPLEMENTED)
*          -o                       Generate an embeddable object (.o file)
*          -cx                      Specify compression algorithm for data (see usage below)
*
*
*   EXAMPLES:

*
*/
    printf("USAGE:\n\n");
    printf("    > rrem [--help] --input <filename.ext>,[otherfile.ext] [--output <filename.ext>]\n");
    printf("           [--comp <value>] [--gen-object]\n");
    
    printf("\nOPTIONS:\n\n");
    printf("    -v, --version                   : Show tool version and info\n");
    printf("    -h, --help                      : Show command line usage help\n");
    printf("    -i, --input <filename.ext>,<otherfile.ext>     : Define input files.\n");
    printf("                                      Supported extensions: .rfx, .sfs, .wav\n");
    printf("    -o, --output <filename.ext>     : Define output file.\n");
    printf("                                      Supported extensions: .wav, .h\n");
    printf("                                      NOTE: If not specified, defaults to: output.wav\n\n");
    printf("    -f, --comp <svalue>             : Define data compression method\n");
    printf("                                    : Format output wave.\n")
    printf("                                          0 - NONE\n");
    printf("                                          1 - DEFLATE\n");
    printf("                                          2 - LZMA\n");
    printf("                                          3 - RLE (custom)\n");
    printf("                                          4 - BZ2\n");
    
    printf("\nEXAMPLES:\n\n");
    printf("    > rrem --input image01.png,image02.jpg,mysound.wav\n");
    printf("        Create 'data.rres' and 'data.h' including those 3 files,\n");
    printf("        uses DEFLATE compression for pixel/wave data.\n");
    printf("    > rrem --input image01.png,image02.bmp --output images.rres\n");
    printf("        Create 'images.rres' and 'images.h' including those 2 files,\n");
    printf("        uses DEFLATE compression for pixel data.\n");
    printf("    > rrem --input image01.png,sound.wav,text.txt\n");
    printf("        Create 'data.rres' and 'data.h' including those 3 files,\n");
    printf("        uses DEFLATE compression for pixel/wave/text data.\n");
#endif
}


static int GetRRESFileType(const char *ext)
{
    int type = -1;
/*
    if ((strcmp(ext,"png")==0) || 
        (strcmp(ext,"jpg")==0) ||
        (strcmp(ext,"bmp")==0) ||
        (strcmp(ext,"tga")==0) ||
        (strcmp(ext,"gif")==0) ||
        (strcmp(ext,"pic")==0) ||
        (strcmp(ext,"psd")==0)) type = RRES_TYPE_IMAGE;         // Image
    else if ((strcmp(ext,"txt")==0) ||
             (strcmp(ext,"csv")==0)  ||
             (strcmp(ext,"info")==0)  ||
             (strcmp(ext,"md")==0)) type = RRES_TYPE_TEXT;      // Text
    else if ((strcmp(ext,"wav")==0) ||
             (strcmp(ext,"ogg")==0)) type = RRES_TYPE_WAVE;     // Audio
    else if (strcmp(ext,"obj")==0) type = RRES_TYPE_MESH;       // Mesh
    else type = RRES_TYPE_RAW;     // Unknown
*/
    return type;
}

// Generate C header data for resource usage
// NOTE: Defines resource name and identifier        
static void GenRRESHeaderFile(const char *rresHeaderName, RRES *resources, int resCount)
{
    FILE *headerFile = fopen(rresHeaderName, "wt");
    
    fprintf(headerFile, "#define NUM_RESOURCES %i\n\n", resCount);

    char *name = NULL;
    char *typeName = NULL;
    char *baseFileName = NULL;
    /*
    for (int i = firstFileArgPos; i < argc; i++)
    {
        baseFileName = GetFileName(argv[i]);
    
        int type = GetRRESFileType(GetExtension(argv[i]));
        
        switch (type)
        {
            case 0: typeName = "IMAGE"; break;
            case 1: typeName = "SOUND"; break;
            case 2: typeName = "MODEL"; break;
            case 3: typeName = "TEXT"; break;
            case 4: typeName = "RAW"; break;
            default: typeName = "UNKNOWN"; break;
        }
    
        name = GetFilenameWithoutEx(baseFileName, '.', '/');      // String should be freed manually
        
        fprintf(headerFile, "#define RES_%s 0x%08x\t\t// Embedded as %s\n", name, resId, typeName);
        
        resId++;
    }
    */
    // free(name);
    // free(typeName);
    // free(baseFileName);
    
    fclose(headerFile);
}

// Generate C object compiled file to embed in executable
static void GenRRESObjectFile(const char *rresFileName)
{
    // If .o file generation is required, then we must generate a data.c file and compile it 
    // OPTION: Include tcc compiler lib in program to do not depend on external programs (that can not exist)

    FILE *rresFile = fopen(rresFileName, "rb");
    FILE *codeFile = fopen("data.c", "wt");
    
    // Get rresFile file size
    fseek(rresFile, 0, SEEK_END);
    int fileSize = ftell(rresFile);
    
    if (fileSize > (32*1024*1024)) printf("WARNING: The file you pretend to embed in the exe is larger than 32Mb!!!\n");
    
    printf("rRES file size: %i\n", fileSize);
    
    fprintf(codeFile, "// This file has been automatically generated by rREM - raylib Resource Embedder\n\n");
    
    fprintf(codeFile, "const unsigned char data[%i] = {\n    ", fileSize);
    
    unsigned char *data = (unsigned char *)malloc(fileSize);
    
    fseek(rresFile, 0, SEEK_SET);
    fread(data, fileSize, 1, rresFile);
    
    int blCounter = 0;		// break line counter
     
    for (int i = 0; i < fileSize-1; i ++)
    {
        blCounter++;
        
        fprintf(codeFile, "0x%.2x, ", data[i]);
        
        if (blCounter >= 24)
        {
            fprintf(codeFile, "\n    ");
            blCounter = 0;
        }
    }
    
    fprintf(codeFile, "0x%.2x };\n", data[fileSize-1]);

    fclose(codeFile);
    fclose(rresFile);
    
    free(data);

    //system("gcc -c data.c");  // Compile resource file into object file
    //remove("data.c");         // Remove .c file
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
