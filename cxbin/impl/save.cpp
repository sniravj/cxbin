#include "cxbin/save.h"
#include "ccglobal/tracer.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"

namespace cxbin
{
	void save(const std::string& fileName, trimesh::TriMesh* model, ccglobal::Tracer* tracer)
	{
		std::string extension = stringutil::extensionFromFileName(fileName, true);
		cxmanager.save(model, fileName, extension, tracer);
	}
}