#include "pluginwrl.h"

#include "stringutil/filenameutil.h"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
	struct normalFace
	{
		trimesh::point normal;
		int index[3];
	};

	struct face
	{
		int vertexIndex[3];
	};

	WrlLoader::WrlLoader()
	{

	}

	WrlLoader::~WrlLoader()
	{

	}

	std::string WrlLoader::expectExtension()
	{
		return "wrl";
	}

	bool WrlLoader::tryLoad(FILE* f, size_t fileSize)
	{
		char buf[128];
		if (!fgets(buf, 128, f))
			return false;

		if (strncmp(buf, "#VRML", 5) != 0)
			return false;

		return true;
	}

	bool WrlLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		std::vector<std::vector<std::vector<int>>> polygons_Indexx;
		std::vector<std::vector<trimesh::point>> pointss;
		std::vector <std::vector <face>> facess;


		std::vector<trimesh::point> points;
		std::vector <face> faces;
		std::vector<normalFace> normalFaces;
		float Matrix4x4[4][4] = { 0 };

		char ch;
		int u;
		trimesh::point apoint;
		std::vector<std::vector<int>> polygons_Index;
		bool bHavePoint=false;

		int fileCallTime = 20;
		int faceCallCount = fileSize / fileCallTime;
		int readCount = 0;
		int nextReportCount = 0;
		auto report = [&nextReportCount, &readCount, &faceCallCount, &fileSize, &tracer](int step)->bool {
			readCount += step;

			if (readCount > nextReportCount)
			{
				if (tracer)
				{
					float r = (float)readCount / (float)fileSize;
					if (r > 1.0f)
						r = 1.0f;

					tracer->progress(0.7f * r);
				}
				
				nextReportCount += faceCallCount;
			}

			return true;
		};

		while (!feof(f))
		{
			ch = fgetc(f);

			report(1);
			while (ch)
			{
				if (ch == '{')//排除纹理顶点数据
				{
					fseek(f, -29, SEEK_CUR);
					char buf2[29 + 1] = { 0 };
					buf2[29] = 0;
					fread(buf2, 1, 29, f);
					std::string  strtemp = buf2;
					if (strtemp.find("TextureCoordinate") != std::string::npos)
					{
						do {
							ch = fgetc(f);
							report(1);
						} while (ch != '}');
					}
				}
				if (ch == '[')//排除纹理索引数据
				{
					fseek(f, -15, SEEK_CUR);
					char buf3[15 + 1] = { 0 };
					buf3[15] = 0;
					fread(buf3, 1, 15, f);
					std::string  strtemp = buf3;
					if (strtemp.find("texCoordIndex") != std::string::npos)
					{
						do {
							ch = fgetc(f);
							report(1);
						} while (ch != ']');
					}
				}

                if (ch == '[')
                {
                    fseek(f, -13, SEEK_CUR);
                    char buf3[13 + 1] = { 0 };
                    buf3[13] = 0;
                    fread(buf3, 1, 13, f);
                    std::string  strtemp = buf3;
                    if (strtemp.find("normalIndex") != std::string::npos)
                    {
                        do {
                            ch = fgetc(f);
                            report(1);
                        } while (ch != ']');
                    }
                }

                if (ch == '[')
                {
                    fseek(f, -12, SEEK_CUR);
                    char buf3[12 + 1] = { 0 };
                    buf3[12] = 0;
                    fread(buf3, 1, 12, f);
                    std::string  strtemp = buf3;
                    if (strtemp.find("colorIndex") != std::string::npos)
                    {
                        do {
                            ch = fgetc(f);
                            report(1);
                        } while (ch != ']');
                    }
                }
                
				if (ch == '[')
				{
					fseek(f, -9, SEEK_CUR);
					char buf[9 + 1] = { 0 };
					buf[9] = 0;
					fread(buf, 1, 9, f);
					std::string  strtemp = buf;
					if (strtemp.find("point") != std::string::npos)
					{
						do
						{
							ch = fgetc(f);
							report(1);
						} while (ch == ' ' || ch == '\n');
						fseek(f, -1, SEEK_CUR);
						do {
							fscanf(f, "%f%f%f", &apoint.x, &apoint.y, &apoint.z);
							points.push_back(apoint);
							ch = fgetc(f);

							report(30);
						} while (ch != ']');

						pointss.push_back(points);
						bHavePoint = true;
						points.clear();
					}
				}
                
                if (ch == '[')
                {
                    fseek(f, -12, SEEK_CUR);
                    char buf[12 + 1] = { 0 };
                    buf[12] = 0;
                    fread(buf, 1, 12, f);
                    std::string strtemp = buf;
                    
                    bool isCoordIndex = strtemp.find("coordIndex") != std::string::npos;
                    if (!isCoordIndex) {
                        isCoordIndex = strtemp.find("Index") != std::string::npos;
                    }
                    
                    if (isCoordIndex && bHavePoint)
                    {
                        std::vector<int> index;
                        ch = fgetc(f);
                        while (ch != ']')
                        {
                            if (ch != ' ' && ch != ',' && ch != '\n')
                            {
                                fseek(f, -1, SEEK_CUR);
                                fscanf(f, "%d", &u);
                                if (u != -1)
                                {
                                    index.push_back(u);
                                }
                                else
                                {
                                    if (index.size() == 3)
                                    {
                                        face aface;
                                        aface.vertexIndex[0] = index[0];
                                        aface.vertexIndex[1] = index[1];
                                        aface.vertexIndex[2] = index[2];
                                        faces.push_back(aface);
                                        index.clear();
                                    }
                                    else if (index.size() == 4)
                                    {
                                        face aface;
                                        aface.vertexIndex[0] = index[0];
                                        aface.vertexIndex[1] = index[1];
                                        aface.vertexIndex[2] = index[2];
                                        faces.push_back(aface);
                                        aface.vertexIndex[0] = index[0];
                                        aface.vertexIndex[1] = index[2];
                                        aface.vertexIndex[2] = index[3];
                                        faces.push_back(aface);
                                        index.clear();
                                    }
                                    else if (index.size() > 4)
                                    {
                                        for (int n = 2; n < index.size(); n++)
                                        {
                                            face aface;
                                            aface.vertexIndex[0] = index[0];
                                            aface.vertexIndex[1] = index[n - 1];
                                            aface.vertexIndex[2] = index[n];
                                            faces.push_back(aface);
                                        }
                                        index.clear();
                                    }
                                    else
                                    {
                                        index.clear();
                                    }
                                }
                            }

                            ch = fgetc(f);

                            report(8);
                        }
                        facess.push_back(faces);
                        faces.clear();
                        polygons_Indexx.push_back(polygons_Index);
                        polygons_Index.clear();
                        bHavePoint = false;
                    }
                }
                
				break;
			}
		}

		float start = 0.7f;
		int nfacetss = facess.size();
		float deltaFace = 0.3f / (float)nfacetss;
		for (int n = 0; n < facess.size(); n++)
		{
			float fStart = start + (float)n * deltaFace;
			int callTimes = 5;
			int faceCount = facess[n].size();
			int callCount = faceCount / callTimes;
			
			trimesh::TriMesh* mesh = new trimesh::TriMesh();
			mesh->faces.reserve(faceCount);
			mesh->vertices.reserve(3 * faceCount);

			for (int nIdx = 0; nIdx < faceCount; nIdx++)
			{
				if (tracer && callCount >= 1 && nIdx % callCount == 1)
				{
					tracer->progress(fStart + deltaFace * (float)nIdx / (float)faceCount);
					if (tracer->interrupt())
						return false;
				}

				trimesh::point vertex0 = pointss[n][facess[n][nIdx].vertexIndex[0]];
				trimesh::point vertex1 = pointss[n][facess[n][nIdx].vertexIndex[1]];
				trimesh::point vertex2 = pointss[n][facess[n][nIdx].vertexIndex[2]];

				////矩阵运算
				//vertex0.x = Matrix4x4[0][0] * vertex0.x + Matrix4x4[0][1] * vertex0.y + Matrix4x4[0][2] * vertex0.z + Matrix4x4[0][3];
				//vertex0.y = Matrix4x4[1][0] * vertex0.x + Matrix4x4[1][1] * vertex0.y + Matrix4x4[1][2] * vertex0.z + Matrix4x4[1][3];
				//vertex0.z = Matrix4x4[2][0] * vertex0.x + Matrix4x4[2][1] * vertex0.y + Matrix4x4[2][2] * vertex0.z + Matrix4x4[2][3];

				//vertex1.x = Matrix4x4[0][0] * vertex1.x + Matrix4x4[0][1] * vertex1.y + Matrix4x4[0][2] * vertex1.z + Matrix4x4[0][3];
				//vertex1.y = Matrix4x4[1][0] * vertex1.x + Matrix4x4[1][1] * vertex1.y + Matrix4x4[1][2] * vertex1.z + Matrix4x4[1][3];
				//vertex1.z = Matrix4x4[2][0] * vertex1.x + Matrix4x4[2][1] * vertex1.y + Matrix4x4[2][2] * vertex1.z + Matrix4x4[2][3];

				//vertex2.x = Matrix4x4[0][0] * vertex2.x + Matrix4x4[0][1] * vertex2.y + Matrix4x4[0][2] * vertex2.z + Matrix4x4[0][3];
				//vertex2.y = Matrix4x4[1][0] * vertex2.x + Matrix4x4[1][1] * vertex2.y + Matrix4x4[1][2] * vertex2.z + Matrix4x4[1][3];
				//vertex2.z = Matrix4x4[2][0] * vertex2.x + Matrix4x4[2][1] * vertex2.y + Matrix4x4[2][2] * vertex2.z + Matrix4x4[2][3];

				int vertexIndex = mesh->vertices.size();
				mesh->vertices.push_back(trimesh::point(vertex0.x, vertex0.y, vertex0.z));
				mesh->vertices.push_back(trimesh::point(vertex1.x, vertex1.y, vertex1.z));
				mesh->vertices.push_back(trimesh::point(vertex2.x, vertex2.y, vertex2.z));
				mesh->faces.push_back(trimesh::TriMesh::Face(vertexIndex, vertexIndex + 1, vertexIndex + 2));
			}
			out.push_back(mesh);
		}

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}

}
