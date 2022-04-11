#include "plugincxbin.h"

#include "cxbin/cx/cxutil.h"
#include "cxbin/cx/writer.h"
#include "cxbin/cx/reader.h"

#include "trimesh2/TriMesh.h"
#include "trimesh2/endianutil.h"

namespace cxbin
{
	CXBinSaver::CXBinSaver()
	{

	}

	CXBinSaver::~CXBinSaver()
	{

	}

	std::string CXBinSaver::expectExtension()
	{
		return "cxbin";
	}

	bool CXBinSaver::save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName)
	{
		return writeCXBin(f, out, tracer);
	}

	CXBinLoader::CXBinLoader()
	{

	}

	CXBinLoader::~CXBinLoader()
	{

	}

	std::string CXBinLoader::expectExtension()
	{
		return "cxbin";
	}

	bool CXBinLoader::tryLoad(FILE* f, unsigned fileSize)
	{
		int _version = -1;
		fread(&_version, sizeof(int), 1, f);

		bool need_swap = trimesh::we_are_big_endian();
		if (need_swap)
			trimesh::swap_int(_version);
		if (_version < 0)
			return false;

		char data[12] = "";
		fread(data, 1, 12, f);

		if (strncmp(data, "\niwlskdfjad", 12) != 0)
			return false;
		return true;
	}

	bool CXBinLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);

		return loadCXBin(f, fileSize, *model, tracer);
	}

}