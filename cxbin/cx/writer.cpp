#include "writer.h"
#include "cxutil.h"
#include "cxbin/convert.h"

#include "trimesh2/TriMesh.h"

#include <zlib.h>
#include <map>


namespace cxbin
{
	bool writeMesh(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		bool success = false;

		// 网格数据块
		{
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
				formartPrint(tracer, "writeMesh write %d %d %d %d.", totalNum, (int)compressNum, vertNum, faceNum);

				int iCompressNum = (int)compressNum;
				fwrite((const char*)&iCompressNum, sizeof(int), 1, out);
				fwrite((const char*)cdata, 1, compressNum, out);
				success = true;
			}

			delete[]data;
			delete[]cdata;
		}

		return success;
	}

	bool writeMaterial(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		bool success = false;

		// 纹理数据块
		{
			int UVNum = mesh->UVs.size();
			int faceUVNum = mesh->faceUVs.size();
			int textureIDNum = mesh->textureIDs.size();
			int materialNum = mesh->materials.size();

			unsigned materialSize = 0;
			std::vector<std::pair<unsigned char*, unsigned>> materialSizeVector;
			for (int i = 0; i < materialNum; i++)
			{
				unsigned size = 0;
				unsigned char* matBuffer = mesh->materials[i].encode(size);
				materialSize += size;
				materialSizeVector.push_back({ matBuffer, size });
			}

			// +1 cause std::string end by '\0'
			unsigned mtlNameLen = mesh->mtlName.length() + 1;

			unsigned mapBufferSize = 0;
			int mapTypeCount = trimesh::Material::TYPE_COUNT;
			unsigned mapBufferSizes[trimesh::Material::TYPE_COUNT];
			for (int i = 0; i < mapTypeCount; i++)
			{
				unsigned bufferSize = sizeof(int) + mesh->map_bufferSize[i] * sizeof(char);
				mapBufferSize += bufferSize;
				mapBufferSizes[i] = mesh->map_bufferSize[i] * sizeof(char);
			}

			int totalNum = UVNum * sizeof(trimesh::vec2) + faceUVNum * sizeof(trimesh::ivec3) + textureIDNum * sizeof(int) + materialSize + mtlNameLen * sizeof(char) + mapBufferSize;
			fwrite((const char*)&totalNum, sizeof(int), 1, out);
			fwrite((const char*)&UVNum, sizeof(int), 1, out);
			fwrite((const char*)&faceUVNum, sizeof(int), 1, out);
			fwrite((const char*)&textureIDNum, sizeof(int), 1, out);
			fwrite((const char*)&materialNum, sizeof(int), 1, out);
			for (int i = 0; i < materialSizeVector.size(); i++)
			{
				fwrite((const char*)&materialSizeVector[i].second, sizeof(int), 1, out);
			}
			fwrite((const char*)&mtlNameLen, sizeof(int), 1, out);
			fwrite((const char*)&mapTypeCount, sizeof(int), 1, out);
			uLong compressNum = compressBound(totalNum);
			unsigned char* data = new unsigned char[totalNum];
			unsigned char* cdata = new unsigned char[compressNum];

			unsigned char* dataPtr = data;
			if (UVNum > 0)
			{
				memcpy(dataPtr, &mesh->UVs.at(0), UVNum * sizeof(trimesh::vec2));
				dataPtr += UVNum * sizeof(trimesh::vec2);
			}
			if (faceUVNum > 0)
			{
				memcpy(dataPtr, &mesh->faceUVs.at(0), faceUVNum * sizeof(trimesh::ivec3));
				dataPtr += faceUVNum * sizeof(trimesh::ivec3);
			}
			if (textureIDNum > 0)
			{
				memcpy(dataPtr, &mesh->textureIDs.at(0), textureIDNum * sizeof(int));
				dataPtr += textureIDNum * sizeof(int);
			}
			if (materialNum > 0)
			{
				for (int i = 0; i < materialSizeVector.size(); i++)
				{
					memcpy(dataPtr, materialSizeVector[i].first, materialSizeVector[i].second);
					dataPtr += materialSizeVector[i].second;
				}
			}
			if (mtlNameLen > 1)
			{
				const char* strData = mesh->mtlName.c_str();
				memcpy(dataPtr, strData, mtlNameLen * sizeof(char));
				dataPtr += mtlNameLen * sizeof(char);
			}
			if (mapBufferSize > 0)
			{
				for (int i = 0; i < mapTypeCount; i++)
				{
					memcpy(dataPtr, &mesh->map_bufferSize[i], sizeof(int));
					dataPtr += sizeof(int);
					if (mapBufferSizes[i] > 0)
					{
						memcpy(dataPtr, mesh->map_buffers[i], mapBufferSizes[i]);
						dataPtr += mapBufferSizes[i];
					}
				}
			}

			if (compress(cdata, &compressNum, data, totalNum) == Z_OK)
			{
				formartPrint(tracer, "writeMaterial write %d %d.", totalNum, (int)compressNum);

				int iCompressNum = (int)compressNum;
				fwrite((const char*)&iCompressNum, sizeof(int), 1, out);
				fwrite((const char*)cdata, 1, compressNum, out);
				success = true;
			}

			delete[]data;
			delete[]cdata;

			for (int i = 0; i < materialNum; i++)
			{
				if (materialSizeVector[i].first)
				{
					delete materialSizeVector[i].first;
					materialSizeVector[i].first = nullptr;
				}
			}
			materialSizeVector.clear();
		}

		return success;
	}

	bool writeCXBin0(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		return true;
	}

	bool writeCXBin1(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		bool success = false;

		success = writeMaterial(out,  mesh, tracer);

		return success;
	}

	bool writeCXBin(FILE* out, trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		bool successFlag = false;
		int headCode = 0;
		fwrite((const char*)&headCode, sizeof(int), 1, out);
		char data[12] = "\niwlskdfjad";
		fwrite(data, 1, 12, out);
		successFlag = writeMesh(out, mesh, tracer);
		fwrite((const char*)&version, sizeof(int), 1, out);
		switch (version)
		{
		case 0:
			successFlag = successFlag && writeCXBin0(out, mesh, tracer);
			break;
		case 1:
			successFlag = successFlag && writeCXBin1(out, mesh, tracer);
			break;
		default:
			successFlag = successFlag && writeCXBin0(out, mesh, tracer);
			break;
		}

		return successFlag;
	}
}