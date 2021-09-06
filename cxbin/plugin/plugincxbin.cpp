#include "plugincxbin.h"

#include "cxbin/cx/cxutil.h"
#include "cxbin/cx/writer.h"
#include "cxbin/cx/reader.h"

#include "trimesh2/TriMesh.h"

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

	bool CXBinSaver::save(trimesh::TriMesh* mesh, const std::string& fileName, ccglobal::Tracer* tracer)
	{
		std::fstream out(fileName, std::ios::out | std::ios::binary);
		bool saveOK = false;
		if (out.is_open())
			saveOK = writeCXBin(out, mesh, tracer);
		out.close();
		return saveOK;
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