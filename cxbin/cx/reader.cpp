#include "reader.h"
#include <zlib.h>
#include <map>

#include "trimesh2/TriMesh.h"
#include "trimesh2/endianutil.h"
#include "cxbin/convert.h"
#include "ccglobal/tracer.h"
#include "imageproc/imageloader.h"


namespace cxbin
{
	bool loadMesh(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;

		// �������ݿ�
		{
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
		}

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

	bool loadMaterial(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;

		// �������ݿ�
		{
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
				trimesh::swap_int(materialNum);
				trimesh::swap_int(mtlNameLen);
				trimesh::swap_int(mapTypeCount);
				trimesh::swap_int(compressNum);
			}
			unsigned char* data = new unsigned char[totalNum];
			unsigned char* cdata = new unsigned char[compressNum];
			formartPrint(tracer, "loadCXBin1 load %d %d %d %d.", totalNum, (int)compressNum, UVNum, faceUVNum);

			uLong uTotalNum = (uLong)totalNum;
			if (fread(cdata, 1, compressNum, f))
			{
				if (uncompress(data, &uTotalNum, cdata, compressNum) == Z_OK)
				{
					unsigned char* dataPtr = data;
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

							delete [] matBuffer;
							matBuffer = nullptr;
						}
					}
					// > 1 cause std::string end by '\0'
					if (mtlNameLen > 1)
					{
						char* strData = new char[mtlNameLen];
						memcpy(strData, dataPtr, mtlNameLen * sizeof(char));
						dataPtr += mtlNameLen * sizeof(char);

						out.mtlName = std::string(strData);

						delete strData;
						strData = nullptr;
					}
					if (mapTypeCount == trimesh::Material::TYPE_COUNT)
					{
						for (int i = 0; i < mapTypeCount; i++)
						{
							int bufferSize;
							memcpy(&bufferSize, dataPtr, sizeof(int));
							dataPtr += sizeof(int);
							unsigned dataSize = bufferSize * sizeof(unsigned char);
							unsigned char* bufferData = nullptr;
							if (dataSize > 0)
							{
								bufferData = new unsigned char[bufferSize];
								memcpy(bufferData, dataPtr, dataSize);
								dataPtr += dataSize;
							}

#ifdef TRIMESH_MAPBUF_RGB
							imgproc::ImageData bufferImage;
							bufferImage.format = imgproc::ImageDataFormat::FORMAT_RGBA_8888;

							if (bufferData)
							{
								imgproc::loadImageFromMem_freeImage(bufferData, dataSize, bufferImage, imgproc::ImageFormat::IMG_FORMAT_PNG);

								delete bufferData;
								bufferData = nullptr;
							}

							out.map_bufferSize[i] = imgproc::encodeWH(bufferImage.width, bufferImage.height);
							out.map_buffers[i] = bufferImage.data;
							bufferImage.data = nullptr;
#else
							out.map_bufferSize[i] = bufferSize;
							out.map_buffers[i] = bufferData;
#endif // TRIMESH_MAPBUF_RGB
						}
					}

					success = true;
				}
			}

			delete[]data;
			delete[]cdata;
		}

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

	// version 0.0
	bool loadCXBin0(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		return true;
	}

	// version 1.0 �ɰ汾
	bool loadCXBin1_old(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
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
					delete strData;
					strData = nullptr;
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

						unsigned char* pngBuffer = nullptr;
						unsigned pngBufferSize = 0;

						if (bufferData)
						{
							imgproc::ImageData bufferImage;
							bufferImage.width = width;
							bufferImage.height = height;
							bufferImage.data = bufferData;
							bufferImage.format = imgproc::ImageDataFormat::FORMAT_RGBA_8888;
							imgproc::writeImage2Mem_freeImage(bufferImage, pngBuffer, pngBufferSize, imgproc::ImageFormat::IMG_FORMAT_PNG);
						}

						out.map_bufferSize[i] = pngBufferSize;
						out.map_buffers[i] = pngBuffer;
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

	// version 1.0
	bool loadCXBin1(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool success = false;

		success = loadMaterial(f, fileSize, out, tracer);

		return success;
	}

	bool loadCXBin(FILE* f, size_t fileSize, trimesh::TriMesh& out, ccglobal::Tracer* tracer)
	{
		bool successFlag = false;

		int headCode = -1;
		fread(&headCode, sizeof(int), 1, f);

		char data[12] = "";
		fread(data, 1, 12, f);

		if (headCode != 1)
		{
			successFlag = loadMesh(f, fileSize, out, tracer);

			int version = -1;
			fread(&version, sizeof(int), 1, f);
			if (feof(f))
				return successFlag;

			bool need_swap = trimesh::we_are_big_endian();
			if (need_swap)
				trimesh::swap_int(version);

			switch (version)
			{
			case 0:
				successFlag = successFlag && loadCXBin0(f, fileSize, out, tracer);
				break;
			case 1:
				successFlag = successFlag && loadCXBin1(f, fileSize, out, tracer);
				break;
			default:
				successFlag = successFlag && loadCXBin0(f, fileSize, out, tracer);
				break;
			}
		}
		// ���� cxbin 1.0 �ɰ汾
		else if (headCode == 1)
		{
			successFlag = loadCXBin1_old(f, fileSize, out, tracer);
		}

		return successFlag;
	}
}