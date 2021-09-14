#ifndef CXBIN_CONVERT_1630772813003_H
#define CXBIN_CONVERT_1630772813003_H
#include "cxbin/interface.h"
#include "ccglobal/tracer.h"
#include "trimesh2/Vec.h"
#include <vector>

namespace trimesh
{
	class TriMesh;
}

namespace cxbin
{
	CXBIN_API void release(std::vector<trimesh::TriMesh*>& models);
	CXBIN_API void tess(const std::vector<trimesh::vec3>& verts, const std::vector<int>& thisface, std::vector<trimesh::ivec3>& tris);

	class CXBIN_API TTracer : public ccglobal::Tracer
	{
	public:
		TTracer(ccglobal::Tracer* _tracer);
		virtual ~TTracer();

		void progress(float r) override;
		bool interrupt() override;

		void message(const char* msg) override;
		void failed(const char* msg) override;
		void success() override;

		bool pass;
		ccglobal::Tracer* _tracer;
	};

	CXBIN_API void formartPrint(ccglobal::Tracer* tracer, const char* format, ...);
}

#endif // CXBIN_CONVERT_1630772813003_H