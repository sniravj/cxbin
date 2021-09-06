#ifndef CXBIN_READER_1630913098880_H
#define CXBIN_READER_1630913098880_H
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
	bool loadCXBin(FILE* f, unsigned fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer);
}

#endif // CXBIN_READER_1630913098880_H