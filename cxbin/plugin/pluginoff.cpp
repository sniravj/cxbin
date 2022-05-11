#include "pluginoff.h"

#include "stringutil/filenameutil.h"
#include "trimesh2/TriMesh.h"
#include "cxbin/impl/inner.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
	OffLoader::OffLoader()
	{

	}

	OffLoader::~OffLoader()
	{

	}

	std::string OffLoader::expectExtension()
	{
		return "off";
	}

	bool OffLoader::tryLoad(FILE* f, size_t fileSize)
	{
		char buf[128];
		if (!fgets(buf, 128, f)) 
			return false;
		
		if (strncmp(buf, "OFF", 3) != 0)
			return false;

		if (!fgets(buf, 128, f))
			return false;

		int nverts, nfaces, unused;
		if (sscanf(buf, "%d %d %d", &nverts, &nfaces, &unused) < 2)
			return false;

		return nverts >= 3;
	}

	bool OffLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		stringutil::skip_comments(f);
		char buf[1024];
		GET_LINE();
		GET_LINE();
		int nverts, nfaces, unused;
		if (sscanf(buf, "%d %d %d", &nverts, &nfaces, &unused) < 2)
			return false;

		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);

		if (!read_verts_asc(f, model, nverts, 3, 0, -1, -1, false, -1))
		{
			if (tracer)
				tracer->failed("read verts asc failed");
			return false;
		}

		if (tracer)
		{
			tracer->progress(0.5f);
		}

		if (tracer && tracer->interrupt())
			return false;

		if (!read_faces_asc(f, model, nfaces, 1, 0, 1, true))
		{
			if (tracer)
				tracer->failed("read faces asc failed");
			return false;
		}

		if (tracer)
		{
			tracer->progress(0.9f);
		}

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}
}