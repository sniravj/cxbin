#include "writer.h"
#include "cxutil.h"
#include "cxbin/convert.h"

#include "trimesh2/TriMesh.h"

#include <zlib.h>
namespace cxbin
{
	bool writeCXBin0(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		bool success = false;
		int vertNum = mesh->vertices.size();
		int faceNum = mesh->faces.size();

		int totalNum = vertNum * sizeof(trimesh::vec3) + faceNum * sizeof(trimesh::ivec3);
		fwrite((const char*)&totalNum, sizeof(int), 1, out);
		fwrite((const char*)&vertNum, sizeof(int), 1, out);
		fwrite((const char*)&faceNum, sizeof(int), 1, out);
		uLong compressNum = compressBound(totalNum);
		unsigned char* data = new unsigned char[totalNum];
		unsigned char* cdata = new unsigned char[compressNum];

		if (vertNum > 0)
			memcpy(data, &mesh->vertices.at(0), vertNum * sizeof(trimesh::vec3));
		if (faceNum > 0)
			memcpy(data + vertNum * sizeof(trimesh::vec3), &mesh->faces.at(0), faceNum * sizeof(trimesh::ivec3));
		if (compress(cdata, &compressNum, data, totalNum) == Z_OK)
		{
			formartPrint(tracer, "writeCXBin0 write %d %d %d %d.", totalNum, (int)compressNum, vertNum, faceNum);

			int iCompressNum = (int)compressNum;
			fwrite((const char*)&iCompressNum, sizeof(int), 1, out);
			fwrite((const char*)cdata, 1, compressNum, out);
			success = true;
		}

		delete[]data;
		delete[]cdata;
		return success;
	}

	bool writeCXBin(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		fwrite((const char*)&version, sizeof(int), 1, out);
		char data[12] = "\niwlskdfjad";
		fwrite(data, 1, 12, out);
		if(version == 0)
			return writeCXBin0(out, mesh, tracer);
		return false;
	}
}