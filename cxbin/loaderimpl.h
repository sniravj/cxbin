#ifndef CXBIN_LOADERIMPL_1630737422772_H
#define CXBIN_LOADERIMPL_1630737422772_H
#include "cxbin/interface.h"
#include <string>
#include <vector>
#include <memory>
namespace ccglobal
{
	class Tracer;
}

namespace trimesh
{
	class TriMesh;
}

namespace cxbin
{

	class LoaderImpl
	{
    public:
        std::string modelPath;
        virtual void associateFileList(FILE* f, ccglobal::Tracer* tracer, const std::string &filePath, std::vector<std::shared_ptr<AssociateFileInfo>>& out);
	public:
		virtual ~LoaderImpl() {}

		virtual std::string expectExtension() = 0;
		virtual bool tryLoad(FILE* f, size_t fileSize) = 0;
		virtual bool load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) = 0;
	};

	CXBIN_API void registerLoaderImpl(LoaderImpl* impl);
	CXBIN_API void unRegisterLoaderImpl(LoaderImpl* impl);
}

#endif // CXBIN_LOADERIMPL_1630737422772_H
