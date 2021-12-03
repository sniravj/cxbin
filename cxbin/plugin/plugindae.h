#ifndef CXBIN_PLUGINDAE_1630741230198_H
#define CXBIN_PLUGINDAE_1630741230198_H
#include "cxbin/loaderimpl.h"

namespace cxbin
{
	class DaeLoader : public LoaderImpl
	{
	public:
		DaeLoader();
		virtual ~DaeLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, unsigned fileSize) override;
		bool load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGINDAE_1630741230198_H
