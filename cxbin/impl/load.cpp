#include "cxbin/load.h"
#include "ccglobal/tracer.h"
#include "cxbin/convert.h"

#include "cxbin/impl/cxbinmanager.h"
#include "stringutil/filenameutil.h"
#include "cxbin/impl/inner.h"
#include <assert.h>



namespace cxbin
{
	void mergeTriMesh(trimesh::TriMesh* outMesh, std::vector<trimesh::TriMesh*>& inMeshes, bool fanzhuan=false);
	trimesh::TriMesh* loadAll(const std::string& fileName, ccglobal::Tracer* tracer)
	{
		std::vector<trimesh::TriMesh*> models = loadT(fileName, tracer);
		if (models.size() <= 0) return nullptr;
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
		std::vector<trimesh::TriMesh*> models;
		FILE* f = fopen(fileName.c_str(), "rb");

		//formartPrint(tracer, "loadT : load file %s", fileName.c_str());
		
		if (!f)
		{
			formartPrint(tracer, "load T : Load file error for [%s]", strerror(errno));
			if (tracer)
				tracer->failed("Open File Error.");
			return models;
		}

		std::string extension = stringutil::extensionFromFileName(fileName, true);
		models = cxmanager.load(f, extension, tracer, fileName);

		if (tracer && models.size() > 0)
		{
			tracer->progress(1.0f);
			tracer->success();
		}

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
}
