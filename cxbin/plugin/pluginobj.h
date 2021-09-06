#ifndef CXBIN_PLUGINOBJ_1630741230194_H
#define CXBIN_PLUGINOBJ_1630741230194_H
#include "cxbin/loaderimpl.h"

namespace cxbin
{
	class ObjLoader : public LoaderImpl
	{
	public:
		ObjLoader();
		virtual ~ObjLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, unsigned fileSize) override;
		bool load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGINOBJ_1630741230194_H