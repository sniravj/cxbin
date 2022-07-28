#include "reader.h"
#include <zlib.h>
#include <map>
#include "trimesh2/TriMesh.h"

#include "cxbin/convert.h"
#include "ccglobal/tracer.h"
#include "trimesh2/endianutil.h"

namespace cxbin
{
	bool loadCXBin0(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;
		int vertNum = 0;
		int faceNum = 0;
		int totalNum = 0;
		int compressNum = 0;

		bool need_swap = trimesh::we_are_big_endian();
		fread(&totalNum, sizeof(int), 1, f);
		fread(&vertNum, sizeof(int), 1, f);
		fread(&faceNum, sizeof(int), 1, f);
		fread(&compressNum, sizeof(int), 1, f);
		if (need_swap)
		{
			trimesh::swap_int(totalNum);
			trimesh::swap_int(vertNum);
			trimesh::swap_int(faceNum);
			trimesh::swap_int(compressNum);
		}
		unsigned char* data = new unsigned char[totalNum];
		unsigned char* cdata = new unsigned char[compressNum];
		formartPrint(tracer, "loadCXBin0 load %d %d %d %d.", totalNum, (int)compressNum, vertNum, faceNum);

		uLong uTotalNum = (uLong)totalNum;
		if (fread(cdata, 1, compressNum, f))
		{
			if (uncompress(data, &uTotalNum, cdata, compressNum) == Z_OK)
			{
				if (vertNum > 0)
				{
					out.vertices.resize(vertNum);
					memcpy(&out.vertices.at(0), data, vertNum * sizeof(trimesh::vec3));
				}
				if (faceNum > 0)
				{
					out.faces.resize(faceNum);
					memcpy(&out.faces.at(0), data + vertNum * sizeof(trimesh::vec3), faceNum * sizeof(trimesh::ivec3));
				}

				success = true;
			}
		}

		delete[]data;
		delete[]cdata;

		if (true == success)
		{
			if (tracer)
			{
				tracer->success();
			}
		}
		else
		{
			if (tracer)
			{
				tracer->failed("file read failed");
			}
		}
		return success;
	}

	bool loadCXBin1(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;
		int vertNum = 0;
		int faceNum = 0;
		int UVNum = 0;
		int faceUVNum = 0;
		int textureIDNum = 0;
		int materialNum = 0;
		int mtlNameLen = 0;
		int mapTypeCount = 0;
		int totalNum = 0;
		int compressNum = 0;
		std::vector<int> materialSizeVector;

		bool need_swap = trimesh::we_are_big_endian();
		fread(&totalNum, sizeof(int), 1, f);
		fread(&vertNum, sizeof(int), 1, f);
		fread(&faceNum, sizeof(int), 1, f);
		fread(&UVNum, sizeof(int), 1, f);
		fread(&faceUVNum, sizeof(int), 1, f);
		fread(&textureIDNum, sizeof(int), 1, f);
		fread(&materialNum, sizeof(int), 1, f);
		for (int i = 0; i < materialNum; i++)
		{
			int size;
			fread(&size, sizeof(int), 1, f);
			materialSizeVector.push_back(size);
		}
		fread(&mtlNameLen, sizeof(int), 1, f);
		fread(&mapTypeCount, sizeof(int), 1, f);
		fread(&compressNum, sizeof(int), 1, f);
		if (need_swap)
		{
			trimesh::swap_int(totalNum);
			trimesh::swap_int(vertNum);
			trimesh::swap_int(faceNum);
			trimesh::swap_int(materialNum);
			trimesh::swap_int(mtlNameLen);
			trimesh::swap_int(mapTypeCount);
			trimesh::swap_int(compressNum);
		}
		unsigned char* data = new unsigned char[totalNum];
		unsigned char* cdata = new unsigned char[compressNum];
		formartPrint(tracer, "loadCXBin0 load %d %d %d %d.", totalNum, (int)compressNum, vertNum, faceNum);

		uLong uTotalNum = (uLong)totalNum;
		if (fread(cdata, 1, compressNum, f))
		{
			if (uncompress(data, &uTotalNum, cdata, compressNum) == Z_OK)
			{
				unsigned char* dataPtr = data;
				if (vertNum > 0)
				{
					out.vertices.resize(vertNum);
					memcpy(&out.vertices.at(0), dataPtr, vertNum * sizeof(trimesh::vec3));
					dataPtr += vertNum * sizeof(trimesh::vec3);
				}
				if (faceNum > 0)
				{
					out.faces.resize(faceNum);
					memcpy(&out.faces.at(0), dataPtr, faceNum * sizeof(trimesh::ivec3));
					dataPtr += faceNum * sizeof(trimesh::ivec3);
				}
				if (UVNum > 0)
				{
					out.UVs.resize(UVNum);
					memcpy(&out.UVs.at(0), dataPtr, UVNum * sizeof(trimesh::vec2));
					dataPtr += UVNum * sizeof(trimesh::vec2);
				}
				if (faceUVNum > 0)
				{
					out.faceUVs.resize(faceUVNum);
					memcpy(&out.faceUVs.at(0), dataPtr, faceUVNum * sizeof(trimesh::ivec3));
					dataPtr += faceUVNum * sizeof(trimesh::ivec3);
				}
				if (textureIDNum > 0)
				{
					out.textureIDs.resize(textureIDNum);
					memcpy(&out.textureIDs.at(0), dataPtr, textureIDNum * sizeof(int));
					dataPtr += textureIDNum * sizeof(int);
				}
				if (materialNum > 0)
				{
					out.materials.resize(materialNum);
					for (int i = 0; i < materialNum; i++)
					{
						int size = materialSizeVector[i];
						unsigned char* matBuffer = new unsigned char[size];
						memcpy(matBuffer, dataPtr, size);
						dataPtr += size;

						out.materials[i].decode(matBuffer, size);

						delete matBuffer;
						matBuffer = nullptr;
					}
				}
				if (mtlNameLen > 0)
				{
					char* strData = new char[mtlNameLen];
					memcpy(strData, dataPtr, mtlNameLen * sizeof(char));
					dataPtr += mtlNameLen * sizeof(char);

					out.mtlName = std::string(strData);
				}
				if (mapTypeCount > 0 && mapTypeCount == trimesh::Material::TYPE_COUNT)
				{
					for (int i = 0; i < mapTypeCount; i++)
					{
						int width, height;
						memcpy(&width, dataPtr, sizeof(int));
						dataPtr += sizeof(int);
						memcpy(&height, dataPtr, sizeof(int));
						dataPtr += sizeof(int);
						unsigned bufferSize = width * height * sizeof(unsigned char);
						unsigned char* bufferData = nullptr;
						if (bufferSize > 0)
						{
							bufferData = new unsigned char[bufferSize];
							memcpy(bufferData, dataPtr, bufferSize);
							dataPtr += bufferSize;
						}

						out.map_widths[i] = width;
						out.map_heights[i] = height;
						out.map_buffers[i] = bufferData;
					}
				}

				success = true;
			}
		}

		delete[]data;
		delete[]cdata;

		if (true == success)
		{
			if (tracer)
			{
				tracer->success();
			}
		}
		else
		{
			if (tracer)
			{
				tracer->failed("file read failed");
			}
		}
		return success;
	}

	bool loadCXBin(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		static std::map<int, std::function<bool(FILE*, size_t, trimesh::TriMesh&, ccglobal::Tracer*)>> verLFuncMap = {
		{0, loadCXBin0},
		{1, loadCXBin1}
		};

		int version = -1;
		fread(&version, sizeof(int), 1, f);

		bool need_swap = trimesh::we_are_big_endian();
		if (need_swap)
			trimesh::swap_int(version);

		char data[12] = "";
		fread(data, 1, 12, f);
		if (version < verLFuncMap.size())
			return verLFuncMap[version](f, fileSize, out, tracer);
		return false;
	}
}