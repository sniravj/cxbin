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
		bool save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName = "") override;
	};

	class CXBinLoader : public LoaderImpl
	{
	public:
		CXBinLoader();
		virtual ~CXBinLoader();

		virtual std::string expectExtension() override;
		virtual bool tryLoad(FILE* f, unsigned fileSize) override;
		virtual bool load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;

	};
}

#endif // CXBIN_PLUGINCXBIN_1630741230195_H