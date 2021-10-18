#include "cxbin/save.h"
#include "ccglobal/tracer.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"

namespace cxbin
{
	bool save(const std::string& fileName, trimesh::TriMesh* model, ccglobal::Tracer* tracer)
	{
		std::string extension = stringutil::extensionFromFileName(fileName, true);
		return cxmanager.save(model, fileName, extension, tracer);
	}
}