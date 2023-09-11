#ifndef CXBIN_LOAD_1630734417275_H
#define CXBIN_LOAD_1630734417275_H
#include "cxbin/interface.h"
#include "trimesh2/TriMesh.h"
#include <vector>
#include <string>

#include "ccglobal/tracer.h"

namespace cxbin
{
	CXBIN_API std::vector<trimesh::TriMesh*> loadT(const std::string& fileName, ccglobal::Tracer* tracer);
	CXBIN_API std::vector<trimesh::TriMesh*> loadT(const char* fileName, ccglobal::Tracer* tracer);
	CXBIN_API trimesh::TriMesh* loadAll(const std::string& fileName, ccglobal::Tracer* tracer);
	CXBIN_API trimesh::TriMesh* loadAll(const char* fileName, ccglobal::Tracer* tracer);
	CXBIN_API std::vector<trimesh::TriMesh*> loadT(int fd, ccglobal::Tracer* tracer);
}

#endif // CXBIN_LOAD_1630734417275_H