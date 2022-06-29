#include "cxbin/save.h"
#include "ccglobal/tracer.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"
#include "cxbin/load.h"

namespace cxbin
{
	bool save(const std::string& fileName, trimesh::TriMesh* model, ccglobal::Tracer* tracer)
	{
		std::string extension = stringutil::extensionFromFileName(fileName, true);
		return cxmanager.save(model, fileName, extension, tracer);
	}

	bool saveWithExtension(const std::string& fileName, const std::string& extension, trimesh::TriMesh* model, ccglobal::Tracer* tracer)
	{
		return cxmanager.save(model, fileName, extension, tracer);
	}

	bool convert(const std::string& fileIn, const std::string& fileOut, ccglobal::Tracer* tracer)
	{
		trimesh::TriMesh* model=loadAll(fileIn, tracer);
		std::string extension = stringutil::extensionFromFileName(fileOut, true);
		return cxmanager.save(model, fileOut, extension, tracer);
	}
}