#include "cxbin/format.h"
#include "cxbin/impl/cxbinmanager.h"
#include<string.h>

namespace cxbin
{
    MeshFileFormat testMeshFileFormat(const std::string& fileName)
    {
        std::string extension = cxmanager.testFormat(fileName);
        return extension2Format(extension);
    }

    MeshFileFormat extension2Format(const std::string& extension)
    {
        if(!strcmp(extension.c_str(), "cxbin"))
            return MESH_FILE_FORMAT_CXBIN;
        else if(!strcmp(extension.c_str(), "stl"))
            return MESH_FILE_FORMAT_STL;
        else if(!strcmp(extension.c_str(), "obj"))
            return MESH_FILE_FORMAT_OBJ;
        else if(!strcmp(extension.c_str(), "ply"))
            return MESH_FILE_FORMAT_PLY;
        else if(!strcmp(extension.c_str(), "off"))
            return MESH_FILE_FORMAT_OFF;
        else if(!strcmp(extension.c_str(), "3mf"))
            return MESH_FILE_FORMAT_3MF;
        else if(!strcmp(extension.c_str(), "dae"))
            return MESH_FILE_FORMAT_DAE;
        else if(!strcmp(extension.c_str(), "3ds"))
            return MESH_FILE_FORMAT_3DS;
        else if(!strcmp(extension.c_str(), "wrl"))
            return MESH_FILE_FORMAT_WRL;

        return MESH_FILE_FORMAT_UNKOWN;
    }

    const char* all_extensions [] = {
            "cxbin",
            "stl",
            "obj",
            "ply",
            "off",
            "3mf",
            "dae",
            "unkown"
    };
    std::string format2Extension(MeshFileFormat format)
    {
        return all_extensions[(int)format];
    }
}