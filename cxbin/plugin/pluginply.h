#ifndef CXBIN_PLUGINPLY_1630741230194_H
#define CXBIN_PLUGINPLY_1630741230194_H
#include "cxbin/loaderimpl.h"

namespace cxbin
{
	class PlyLoader : public LoaderImpl
	{
	public:
		PlyLoader();
		virtual ~PlyLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, unsigned fileSize) override;
		bool load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGINPLY_1630741230194_H