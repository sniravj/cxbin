#ifndef CXBIN_CONVERT_1630772813003_H
#define CXBIN_CONVERT_1630772813003_H
#include "cxbin/interface.h"
#include "trimesh2/Vec.h"
#include <vector>

namespace trimesh
{
	class TriMesh;
}

namespace cxbin
{
	CXBIN_API void release(std::vector<trimesh::TriMesh*>& models);

	CXBIN_API void tess(const std::vector<trimesh::vec3>& verts, const std::vector<int>& thisface, std::vector<trimesh::ivec3>& tris);
}

#endif // CXBIN_CONVERT_1630772813003_H