/**********************************************************************************************
*
*   rres-raylib v1.0 - rres loaders specific to raylib data structures
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2020 Ramon Santamaria (@raysan5)
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
**********************************************************************************************/

#ifndef RRES-RAYLIB_H
#define RRES-RAYLIB_H

#include "rres.h"
#include "raylib.h"

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

RRESDEF char *LoadRawFromRRES(RRESData rres);
RRESDEF char *LoadTextFromRRES(RRESData rres);
RRESDEF Image LoadImageFromRRES(RRESData rres);
RRESDEF Wave LoadWaveFromRRES(RRESData rres);
RRESDEF Font LoadFontFromRRES(RRESData *rres, int count);
RRESDEF Mesh LoadMeshFromRRES(RRESData *rres, int count);
//RRESDEF Material LoadMaterialFromRRES(RRESData rres);
//RRESDEF Model LoadModelFromRRES(RRESData *rres, int count);
//RRESDEF ModelAnimation LoadModelAnimationFromRRES(RRESData *rres, int count);


#endif // RRES_H

/***********************************************************************************
*
*   RRES IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES-RAYLIB_IMPLEMENTATION)

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// QUESTION: How to load each type of data from RRES -> Custom implementations (library dependant)

char *LoadRawFromRRES(RRESData rres)
{
    char *data = NULL;
    
    if (rres.type == RRES_TYPE_RAW) data = (char *)rres.data;

    return data;
}

Image LoadImageFromRRES(RRESData rres)
{
    Image image = { 0 };
    
    if (rres.type == RRES_TYPE_IMAGE)
    {
        image.width = rres.props[0];
        image.height = rres.props[1];
        image.mipmaps = rres.props[2];
        image.format =  rres.props[3];
        
        image.data = rres.data;     // WARNING: Pointer assignment! -> Shallow copy?
    }
    
    return image;
}

Wave LoadWaveFromRRES(RRESData rres)
{
    Wave wave = { 0 };
    
    if ((rres.count == 1) && (rres.type == RRES_TYPE_WAVE))
    {
        wave.sampleCount = rres.chunk[0].properties[0];
        wave.sampleRate = rres.chunk[0].properties[1];
        wave.sampleSize = rres.chunk[0].properties[2];
        wave.channels = rres.chunk[0].properties[3];
        
        wave.data = rres.chunk[0].data;     // WARNING: Pointer assignment! -> Shallow copy?
    }
}

Font LoadFontFromRRES(RRESData *rres, int count)
{
    Font font = { 0 };

    if ((rres.count == 2) && (rres.type == RRES_TYPE_FONT))

    // To load a Font we expect at least 2 resource data packages
    if (rres[0].type == RRES_TYPE_FONT_IMAGE)
    {
        Image image = LoadImageFromRRES(rres[0]);
        font.texture = LoadTextureFromImage(image);
    }
    
    if (rres[1].type == RRES_TYPE_FONT_CHARDATA) //-> RAW DATA?
    {
        font.charsCount = rres[1].properties[0];
        font.baseSize = rres[1].properties[1];
        
        // Font rectangle properties come as individual properties
        font.recs = (Rectangle *)RRES_MALLOC(font.charsCount*sizeof(Rectangle));
        
        for (int i = 0; i < font.charsCount; i++)
        {
            font.recs[i].x = (float)rres[1].properties[2 + i*4];
            font.recs[i].y = (float)rres[1].properties[2 + i*4 + 1];
            font.recs[i].width = (float)rres[1].properties[2 + i*4 + 2];
            font.recs[i].height = (float)rres[1].properties[2 + i*4 + 3];
        }
        
        font.chars = (CharInfo *)rres[1]->data;
    }

    return font;
}

Mesh LoadMeshFromRRES(RRESData *rres, int count)
{
    Mesh mesh = { 0 }

    mesh.vertexCount = rres[0].props[0];
    
    // TODO: Make sure all rres[i] vertexCount are the same (requried by raylib)
    
    for (int i = 0; i < count; i++)
    {
        if (rres[i].type == RRES_TYPE_VERTEX)
        {
            switch (rres[i].props[1])
            {
                case RRES_VERT_POSITION: if (rres[i].props[2] == RRES_VERT_FORMAT_FLOAT) mesh.vertices = (float *)rres[i]->data; break;
                case RRES_VERT_TEXCOORD1: if (rres[i].props[2] == RRES_VERT_FORMAT_FLOAT) mesh.texcoords = (float *)rres[i]->data; break;
                case RRES_VERT_TEXCOORD2: if (rres[i].props[2] == RRES_VERT_FORMAT_FLOAT) mesh.texcoords2 = (float *)rres[i]->data; break;
                case RRES_VERT_TEXCOORD3: break;    // Not supported by raylib mesh format
                case RRES_VERT_TEXCOORD4: break;    // Not supported by raylib mesh format
                case RRES_VERT_NORMAL: if (rres[i].props[2] == RRES_VERT_FORMAT_FLOAT) mesh.normals = (float *)rres[i]->data; break;
                case RRES_VERT_TANGENT: if (rres[i].props[2] == RRES_VERT_FORMAT_FLOAT) mesh.tangents = (float *)rres[i]->data; break;
                case RRES_VERT_COLOR: if (rres[i].props[2] == RRES_VERT_FORMAT_BYTE) mesh.colors = (float *)rres[i]->data; break;
                case RRES_VERT_INDEX: if (rres[i].props[2] == RRES_VERT_FORMAT_SHORT) mesh.indices = (float *)rres[i]->data; break;
                default: break;
            }
        }
    }
    
    return mesh;
}

// NOTE: Text must be '\0' terminated
char *LoadTextFromRRES(RRESData rres)
{
    char *text = NULL;
    
    if (rres.type == RRES_TYPE_TEXT)
    {
        text = (char *)rres.data;       // WARNING: Do a copy of data?
        
        if (((char *)rres.data)[rres.props[0] - 1] != '\0') TRACELOG(LOG_WARNING, "Text data is not '\0' terminated!") 
    }

    return text;
}

#endif // RRES-RAYLIB_IMPLEMENTATION