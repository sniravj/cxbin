#ifndef CXBIN_PLUGIN3DS_1630741230196_H
#define CXBIN_PLUGIN3DS_1630741230196_H
#include "cxbin/loaderimpl.h"

namespace cxbin
{
	class _3dsLoader : public LoaderImpl
	{
	public:
		_3dsLoader();
		virtual ~_3dsLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, unsigned fileSize) override;
		bool load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGIN3DS_1630741230196_H