#ifndef CXBIN_SAVE_1630734417276_H
#define CXBIN_SAVE_1630734417276_H
#include "cxbin/interface.h"
#include <string>

namespace ccglobal
{
	class Tracer;
}

namespace trimesh
{
	class TriMesh;
}

namespace cxbin
{
	CXBIN_API void save(const std::string& fileName, trimesh::TriMesh* model, ccglobal::Tracer* tracer);
}
#endif // CXBIN_SAVE_1630734417276_H