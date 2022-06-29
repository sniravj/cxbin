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
	CXBIN_API bool save(const std::string& fileName, trimesh::TriMesh* model, ccglobal::Tracer* tracer);

	//support extensions : ply, stl, cxbin
	CXBIN_API bool saveWithExtension(const std::string& fileName, const std::string& extension, trimesh::TriMesh* model, ccglobal::Tracer* tracer);

	CXBIN_API bool convert(const std::string& fileIn, const std::string& fileOut, ccglobal::Tracer* tracer);
}
#endif // CXBIN_SAVE_1630734417276_H