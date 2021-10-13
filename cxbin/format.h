#ifndef CXBIN_FORMAT_1634100265837_H
#define CXBIN_FORMAT_1634100265837_H
#include "cxbin/interface.h"
#include <string>

namespace cxbin
{
    enum MeshFileFormat
    {
        MESH_FILE_FORMAT_CXBIN                        = 0,
        MESH_FILE_FORMAT_STL                          = 1,
        MESH_FILE_FORMAT_OBJ                          = 2,
        MESH_FILE_FORMAT_PLY                          = 3,
        MESH_FILE_FORMAT_OFF                          = 4,
        MESH_FILE_FORMAT_3MF                          = 5,
        MESH_FILE_FORMAT_DAE                          = 6,
        MESH_FILE_FORMAT_3DS                          = 7,
        MESH_FILE_FORMAT_WRL                          = 8,
        MESH_FILE_FORMAT_UNKOWN                       = 9
    };

    CXBIN_API MeshFileFormat testMeshFileFormat(const std::string& fileName);

    CXBIN_API MeshFileFormat extension2Format(const std::string& extension);
    CXBIN_API std::string format2Extension(MeshFileFormat format);
}

#endif // CXBIN_FORMAT_1634100265837_H