#ifndef CXBIN_PLUGINCXBIN_1630741230195_H
#define CXBIN_PLUGINCXBIN_1630741230195_H
#include "cxbin/saverimpl.h"
#include "cxbin/loaderimpl.h"

namespace cxbin
{
	class CXBinSaver : public SaverImpl
	{
	public:
		CXBinSaver();
		virtual ~CXBinSaver();

		std::string expectExtension() override;
		bool save(trimesh::TriMesh* mesh, const std::string& fileName, ccglobal::Tracer* tracer) override;
	};

	class CXBinLoader : public LoaderImpl
	{
	public:
		CXBinLoader();
		virtual ~CXBinLoader();

		virtual std::string expectExtension() = 0;
		virtual bool tryLoad(FILE* f, unsigned fileSize) = 0;
		virtual bool load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) = 0;

	};
}

#endif // CXBIN_PLUGINCXBIN_1630741230195_H