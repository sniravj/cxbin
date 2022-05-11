#ifndef CXBIN_PLUGINPLY_1630741230194_H
#define CXBIN_PLUGINPLY_1630741230194_H
#include "cxbin/loaderimpl.h"
#include "cxbin/saverimpl.h"

namespace cxbin
{
	class PlySaver : public SaverImpl
	{
	public:
		PlySaver();
		virtual ~PlySaver();

		std::string expectExtension() override;
		bool save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName = "") override;
	};

	class PlyLoader : public LoaderImpl
	{
	public:
		PlyLoader();
		virtual ~PlyLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, size_t fileSize) override;
		bool load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGINPLY_1630741230194_H