#ifndef CXBIN_PLUGIN3MF_1630741230197_H
#define CXBIN_PLUGIN3MF_1630741230197_H
#include "cxbin/loaderimpl.h"

namespace cxbin
{
	class _3mfLoader : public LoaderImpl
	{
	public:
		_3mfLoader();
		virtual ~_3mfLoader();

		std::string expectExtension() override;
		bool tryLoad(FILE* f, size_t fileSize) override;
		bool load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer) override;
	};
}

#endif // CXBIN_PLUGIN3MF_1630741230197_H