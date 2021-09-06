#include "writer.h"
#include "cxutil.h"

#include "trimesh2/TriMesh.h"

#include <zlib.h>
namespace cxbin
{
	bool writeCXBin0(std::fstream& out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		bool success = false;
		int vertNum = mesh->vertices.size();
		int faceNum = mesh->faces.size();

		int totalNum = vertNum * sizeof(trimesh::vec3) + faceNum * sizeof(trimesh::ivec3);
		out.write((const char*)&totalNum, sizeof(int));
		out.write((const char*)&vertNum, sizeof(int));
		out.write((const char*)&faceNum, sizeof(int));
		uLong compressNum = compressBound(totalNum);
		out.write((const char*)&compressNum, sizeof(uLong));
		unsigned char* data = new unsigned char[totalNum];
		unsigned char* cdata = new unsigned char[compressNum];

		if (vertNum > 0)
			memcpy(data, &mesh->vertices.at(0), vertNum * sizeof(trimesh::vec3));
		if (faceNum > 0)
			memcpy(data + vertNum * sizeof(trimesh::vec3), &mesh->faces.at(0), faceNum * sizeof(trimesh::ivec3));
		if (compress(cdata, &compressNum, data, totalNum))
		{
			out.write((const char*)cdata, compressNum);
			success = true;
		}
		delete[]data;
		delete[]cdata;
		return success;
	}

	bool writeCXBin(std::fstream& out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		out.write((const char*)&version, sizeof(int));

		char data[12] = "\niwlskdfjad";
		out.write(data, 12);
		if(version == 0)
			writeCXBin0(out, mesh, tracer);
		return false;
	}
}