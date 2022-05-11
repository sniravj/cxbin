#ifndef CXBIN_PLUGINSTL_1630741230193_H
#define CXBIN_PLUGINSTL_1630741230193_H
#include "cxbin/loaderimpl.h"
#include "cxbin/saverimpl.h"

namespace cxbin
{
	class StlSaver : public SaverImpl
	{
	public:
		StlSaver();
		virtual ~StlSaver();

		std::string expectExtension() override;
		bool save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName = "") override;
	};

	class StlLoader : public LoaderImpl
	{
	public:
		StlLoader();
		virtual ~StlLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, size_t fileSize) override;
		bool load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGINSTL_1630741230193_H