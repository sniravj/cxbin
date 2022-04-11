#ifndef CXBIN_SAVERIMPL_1630737422772_H
#define CXBIN_SAVERIMPL_1630737422772_H
#include "cxbin/interface.h"
#include <string>

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
	class SaverImpl
	{
	public:
		virtual ~SaverImpl() {}

		virtual std::string expectExtension() = 0;
		virtual bool save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName = "") = 0;
	};

	CXBIN_API void registerSaverImpl(SaverImpl* impl);
	CXBIN_API void unRegisterSaverImpl(SaverImpl* impl);
}

#endif // CXBIN_SAVERIMPL_1630737422772_H