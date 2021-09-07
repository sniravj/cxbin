#include "cxbin/load.h"
#include "ccglobal/tracer.h"
#include "cxbin/convert.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"

namespace cxbin
{
	std::vector<trimesh::TriMesh*> loadT(const std::string& fileName, ccglobal::Tracer* tracer)
	{
		FILE* f = fopen(fileName.c_str(), "rb");

		std::string extension = stringutil::extensionFromFileName(fileName, true);

		std::vector<trimesh::TriMesh*> models = cxmanager.load(f, extension, tracer);
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