#ifndef CXBIN_WRITER_1630913098880_H
#define CXBIN_WRITER_1630913098880_H
#include <fstream>

namespace trimesh
{
	class TriMesh;
}

namespace ccglobal
{
	class Tracer;
}

namespace cxbin
{
	bool writeCXBin(std::fstream& out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer);
}

#endif // CXBIN_WRITER_1630913098880_H