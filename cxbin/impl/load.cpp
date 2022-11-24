#include "cxbin/load.h"
#include "ccglobal/tracer.h"
#include "cxbin/convert.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"
#include "cxbin/impl/inner.h"
#include <assert.h>

#include "boost/nowide/cstdio.hpp"

namespace cxbin
{
	void mergeTriMeshUVSingle(trimesh::TriMesh* outMesh, trimesh::TriMesh* inmesh, bool fanzhuan);
	void mergeTriMeshSingle(trimesh::TriMesh* outMesh, trimesh::TriMesh* inmesh, bool fanzhuan);
	void mergeTriMesh(trimesh::TriMesh* outMesh, std::vector<trimesh::TriMesh*>& inMeshes, bool fanzhuan=false);
	trimesh::TriMesh* loadAll(const std::string& fileName, ccglobal::Tracer* tracer)
	{
		return loadAll(fileName.c_str(), tracer);
	}

	trimesh::TriMesh* loadAll(const char* fileName, ccglobal::Tracer* tracer)
	{
		std::vector<trimesh::TriMesh*> models = loadT(fileName, tracer);
		if (models.size() <= 0)
			return nullptr;

		if (models.size() == 1)
			return models.at(0);

		trimesh::TriMesh* outMesh = new trimesh::TriMesh();
		mergeTriMesh(outMesh, models);
		for (auto& mesh : models)
		{
			delete mesh;
		}
		return outMesh;
	}

	std::vector<trimesh::TriMesh*> loadT(const std::string& fileName, ccglobal::Tracer* tracer)
	{
        return loadT(fileName.c_str(), tracer);
	}

	std::vector<trimesh::TriMesh*> loadT(const char* fileName, ccglobal::Tracer* tracer)
	{
		std::vector<trimesh::TriMesh*> models;
		FILE* f = boost::nowide::fopen(fileName, "rb");

		formartPrint(tracer, "loadT : load file %s", fileName);

		if (!f)
		{
			formartPrint(tracer, "load T : Load file error for [%s]", strerror(errno));
			if (tracer)
				tracer->failed("Open File Error.");
			return models;
		}

		const std::string _fileName = fileName;
		std::string extension = stringutil::extensionFromFileName(_fileName, true);
		models = cxmanager.load(f, extension, tracer, _fileName);

		if (tracer && models.size() > 0)
		{
            for (trimesh::TriMesh* m : models)
                m->clear_normals();

			tracer->progress(1.0f);
			tracer->success();
		}

        //clear normal
        for (trimesh::TriMesh* m : models)
            m->clear_normals();

		if(f)
			fclose(f);

		if (tracer && models.size() == 0)
			tracer->failed("Parse File Error.");
		return models;
	}

	std::vector<trimesh::TriMesh*> loadT(int fd, ccglobal::Tracer* tracer)
	{
		FILE* f = fdopen(fd, "rb");       /*�ļ�������ת��Ϊ�ļ�ָ��*/
		return cxmanager.load(f, "", tracer, "");
	}

	void mergeTriMesh(trimesh::TriMesh* outMesh, std::vector<trimesh::TriMesh*>& inMeshes, bool fanzhuan)
	{
		assert(outMesh);
		size_t totalVertexSize = outMesh->vertices.size();
		size_t totalUVSize = outMesh->cornerareas.size();
		size_t totalTriangleSize = outMesh->faces.size();

		size_t addVertexSize = 0;
		size_t addTriangleSize = 0;
		size_t addUVSize = 0;

		size_t meshSize = inMeshes.size();
		for (size_t i = 0; i < meshSize; ++i)
		{
			if (inMeshes.at(i))
			{
				addVertexSize += inMeshes.at(i)->vertices.size();
				addTriangleSize += inMeshes.at(i)->faces.size();
				addUVSize += inMeshes.at(i)->cornerareas.size();
			}
		}
		totalVertexSize += addVertexSize;
		totalTriangleSize += addTriangleSize;
		totalUVSize += addUVSize;

		if (addVertexSize > 0 && addTriangleSize > 0)
		{
			outMesh->vertices.reserve(totalVertexSize);
			outMesh->cornerareas.reserve(totalUVSize);
			outMesh->faces.reserve(totalTriangleSize);

			size_t startFaceIndex = outMesh->faces.size();
			size_t startVertexIndex = outMesh->vertices.size();;
			size_t startUVIndex = outMesh->cornerareas.size();
			for (size_t i = 0; i < meshSize; ++i)
			{
				trimesh::TriMesh* mesh = inMeshes.at(i);
				if (mesh)
				{
					int vertexNum = (int)mesh->vertices.size();
					int faceNum = (int)mesh->faces.size();
					int uvNum = (int)mesh->cornerareas.size();
					if (vertexNum > 0 && faceNum > 0)
					{
						outMesh->vertices.insert(outMesh->vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
						outMesh->cornerareas.insert(outMesh->cornerareas.end(), mesh->cornerareas.begin(), mesh->cornerareas.end());
						outMesh->faces.insert(outMesh->faces.end(), mesh->faces.begin(), mesh->faces.end());

						size_t endFaceIndex = startFaceIndex + faceNum;
						if (startVertexIndex > 0)
						{
							for (size_t ii = startFaceIndex; ii < endFaceIndex; ++ii)
							{
								trimesh::TriMesh::Face& face = outMesh->faces.at(ii);
								for (int j = 0; j < 3; ++j)
									face[j] += startVertexIndex;

								if (fanzhuan)
								{
									int t = face[1];
									face[1] = face[2];
									face[2] = t;
								}
							}
						}

						startFaceIndex += faceNum;
						startVertexIndex += vertexNum;
						startUVIndex += uvNum;

					}
				}
			}
		}
	}
	void mergeTriMeshUVSingle(trimesh::TriMesh* outMesh, trimesh::TriMesh* inmesh, bool fanzhuan)
	{
		assert(outMesh);
		if (inmesh != nullptr)
		{
			size_t startMaterialSize = outMesh->materials.size();
			size_t startUVSize = outMesh->UVs.size();
			size_t startfaceUVSize = outMesh->faceUVs.size();

			int addMaterialSize = inmesh->materials.size();
			int addfaceUVSize = inmesh->faceUVs.size();
			int addUVSize = inmesh->UVs.size();
			if (addfaceUVSize > 0 && addUVSize > 0)
			{
				outMesh->materials.insert(outMesh->materials.end(), inmesh->materials.begin(), inmesh->materials.end());
				outMesh->faceUVs.insert(outMesh->faceUVs.end(), inmesh->faceUVs.begin(), inmesh->faceUVs.end());
				outMesh->UVs.insert(outMesh->UVs.end(), inmesh->UVs.begin(), inmesh->UVs.end());
				outMesh->textureIDs.insert(outMesh->textureIDs.end(), inmesh->textureIDs.begin(), inmesh->textureIDs.end());

				for (size_t ii = startfaceUVSize; ii < startfaceUVSize + addfaceUVSize; ++ii)
				{
					trimesh::TriMesh::Face& faceUV = outMesh->faceUVs.at(ii);
					int& textureID = outMesh->textureIDs.at(ii);//textureIDs.size() ==faceUVs.size()
					textureID += addMaterialSize;//对应m_materials中的index值

					for (int j = 0; j < 3; ++j)
						faceUV[j] += startfaceUVSize;

					if (fanzhuan)
					{
						int t = faceUV[1];
						faceUV[1] = faceUV[2];
						faceUV[2] = t;
					}
				}
				//material.index
				for (size_t ii = startMaterialSize; ii < startMaterialSize + addMaterialSize; ii++)
				{
					trimesh::Material& material = outMesh->materials.at(ii);
					material.index += addMaterialSize;//对应m_materials中的值

				}
				//此处不合理，待后续完善
				for (int index = 0; index < trimesh::Material::MapType::TYPE_COUNT; index++)
				{
					outMesh->map_bufferSize[index] = inmesh->map_bufferSize[index];
					outMesh->map_buffers[index] = inmesh->map_buffers[index];
				}

			}
		}
	}
	void mergeTriMeshSingle(trimesh::TriMesh* outMesh, trimesh::TriMesh* inmesh, bool fanzhuan)
	{
		assert(outMesh);
		if (inmesh != nullptr)
		{
			size_t startFaceIndex = outMesh->faces.size();
			size_t startVertexIndex = outMesh->vertices.size();;
			size_t startcornerareaNum = outMesh->cornerareas.size();
			int addvertexNum = inmesh->vertices.size();
			int addfaceNum = inmesh->faces.size();
			int addcornerareaNum = inmesh->cornerareas.size();
			if (addvertexNum > 0 && addfaceNum > 0)
			{
				outMesh->vertices.insert(outMesh->vertices.end(), inmesh->vertices.begin(), inmesh->vertices.end());
				outMesh->cornerareas.insert(outMesh->cornerareas.end(), inmesh->cornerareas.begin(), inmesh->cornerareas.end());
				outMesh->faces.insert(outMesh->faces.end(), inmesh->faces.begin(), inmesh->faces.end());

				for (size_t ii = startFaceIndex; ii < startFaceIndex + addfaceNum; ++ii)
				{
					trimesh::TriMesh::Face& face = outMesh->faces.at(ii);
					for (int j = 0; j < 3; ++j)
						face[j] += startVertexIndex;

					if (fanzhuan)
					{
						int t = face[1];
						face[1] = face[2];
						face[2] = t;
					}
				}
			}
		}
	}
}
