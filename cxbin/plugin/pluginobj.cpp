#include "pluginobj.h"

#include "stringutil/filenameutil.h"
#include "trimesh2/TriMesh.h"
#include "cxbin/convert.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
	ObjLoader::ObjLoader()
	{

	}

	ObjLoader::~ObjLoader()
	{

	}

	std::string ObjLoader::expectExtension()
	{
		return "obj";
	}

	bool ObjLoader::tryLoad(FILE* f, unsigned fileSize)
	{
		bool isObj = false;
		int times = 0;
		while (1) {
			stringutil::skip_comments(f);
			if (feof(f))
				break;
			char buf[1024];
			GET_LINE();
			if (LINE_IS("v ") || LINE_IS("v\t")) {
				float x, y, z;
				if (sscanf(buf + 1, "%f %f %f", &x, &y, &z) != 3) {
					break;
				}

				++times;
				if (times >= 3)
				{
					isObj = true;
					break;
				}
			}
		}

		return isObj;
	}

	bool ObjLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);
		unsigned count = 0;
		unsigned calltime = fileSize / 10;
		int num = 0;
		std::vector<int> thisface;
		while (1) {
			if (tracer && tracer->interrupt())
				return false;

			stringutil::skip_comments(f);
			if (feof(f))
				break;
			char buf[1024];
			GET_LINE();

			std::string str = buf;
			count += str.length();
			if (tracer && count >= calltime)
			{
				count = 0;
				num++;
				tracer->progress((float)num / 10.0);
			}
			
			if (LINE_IS("v ") || LINE_IS("v\t")) {
				float x, y, z;
				if (sscanf(buf + 1, "%f %f %f", &x, &y, &z) != 3) {
					if (tracer)
						tracer->failed("sscanf failed");
					return false;
				}
				model->vertices.push_back(trimesh::point(x, y, z));
			}
			else if (LINE_IS("vn ") || LINE_IS("vn\t")) {
				float x, y, z;
				if (sscanf(buf + 2, "%f %f %f", &x, &y, &z) != 3) {
					if (tracer)
						tracer->failed("sscanf failed");
					return false;
				}
				model->normals.push_back(trimesh::vec(x, y, z));
			}
			else if (LINE_IS("f ") || LINE_IS("f\t") ||
				LINE_IS("t ") || LINE_IS("t\t")) {
				thisface.clear();
				char* c = buf;
				while (1) {
					if (tracer && tracer->interrupt())
						return false;

					while (*c && *c != '\n' && !isspace(*c))
						c++;
					while (*c && isspace(*c))
						c++;
					int thisf;
					if (sscanf(c, " %d", &thisf) != 1)
						break;
					if (thisf < 0)
						thisf += model->vertices.size();
					else
						thisf--;
					thisface.push_back(thisf);
				}
				tess(model->vertices, thisface, model->faces);
			}
		}

		// XXX - FIXME
		// Right now, handling of normals is fragile: we assume that
		// if we have the same number of normals as vertices,
		// the file just uses per-vertex normals.  Otherwise, we can't
		// handle it.
		if (model->vertices.size() != model->normals.size())
			model->normals.clear();

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}
}