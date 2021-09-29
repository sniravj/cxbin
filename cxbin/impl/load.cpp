#include "cxbin/load.h"
#include "ccglobal/tracer.h"
#include "cxbin/convert.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"
#include "cxbin/impl/inner.h"

namespace cxbin
{
	std::vector<trimesh::TriMesh*> loadT(const std::string& fileName, ccglobal::Tracer* tracer)
	{
		std::vector<trimesh::TriMesh*> models;
		FILE* f = fopen(fileName.c_str(), "rb");

		formartPrint(tracer, "loadT : load file %s", fileName.c_str());
		
		if (!f)
		{
			formartPrint(tracer, "load T : load file error for [%s]", strerror(errno));
			return models;
		}

		std::string extension = stringutil::extensionFromFileName(fileName, true);
		models = cxmanager.load(f, extension, tracer);

		if (tracer && models.size() > 0)
		{
			tracer->progress(1.0f);
			tracer->success();
		}

		if(f)
			fclose(f);
		return models;
	}

	std::vector<trimesh::TriMesh*> loadT(int fd, ccglobal::Tracer* tracer)
	{
		FILE* f = fdopen(fd, "rb");       /*文件描述符转换为文件指针*/
		return cxmanager.load(f, "", tracer);
	}
}