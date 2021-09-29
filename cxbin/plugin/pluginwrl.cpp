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

	bool WrlLoader::tryLoad(FILE* f, unsigned fileSize)
	{
		char buf[128];
		if (!fgets(buf, 128, f))
			return false;

		if (strncmp(buf, "#VRML", 5) != 0)
			return false;

		return true;
	}

	bool WrlLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
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
		while (!feof(f))
		{
			if (tracer && tracer->interrupt())
				return false;

			ch = fgetc(f);
			while (ch)
			{
				if (ch == '{')//排除纹理顶点数据
				{
					fseek(f, -19, SEEK_CUR);
					char* buf2 = new char[19 + 1];
					buf2[13] = 0;
					fread(buf2, 1, 19, f);
					std::string  strtemp = buf2;
					if (strtemp.find("TextureCoordinate") != std::string::npos)
					{
						do {
							ch = fgetc(f);
						} while (ch != '}');
					}
				}
				if (ch == '[')//排除纹理索引数据
				{
					fseek(f, -15, SEEK_CUR);
					char* buf3 = new char[15 + 1];
					buf3[15] = 0;
					fread(buf3, 1, 15, f);
					std::string  strtemp = buf3;
					if (strtemp.find("texCoordIndex") != std::string::npos)
					{
						do {
							ch = fgetc(f);
						} while (ch != ']');
					}
				}


				if (ch == '[')
				{
					fseek(f, -9, SEEK_CUR);
					char* buf = new char[9 + 1];
					buf[9] = 0;
					fread(buf, 1, 9, f);
					std::string  strtemp = buf;
					if (strtemp.find("point") != std::string::npos)
					{
						do
						{
							ch = fgetc(f);
						} while (ch == ' ' || ch == '\n');
						fseek(f, -1, SEEK_CUR);
						do {
							fscanf(f, "%f%f%f", &apoint.x, &apoint.y, &apoint.z);
							points.push_back(apoint);
							ch = fgetc(f);
						} while (ch != ']');

						pointss.push_back(points);
						points.clear();
					}

					if (strtemp.find("Index") != std::string::npos)
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
						}
						facess.push_back(faces);
						faces.clear();
						polygons_Indexx.push_back(polygons_Index);
						polygons_Index.clear();
					}
				}
				break;
			}
		}

		if (tracer)
		{
			tracer->progress(0.5f);
		}

		int nfacetss = facess.size();
		int calltime = nfacetss / 10;
		if (calltime <= 0)
			calltime = nfacetss;
		for (int n = 0; n < facess.size(); n++)
		{
			if (tracer)
			{
				tracer->progress((float)n / (float)nfacetss);
			}
			if (tracer && n % calltime == 1)
			{
				tracer->progress(0.5f+(float)n / ((float)nfacetss * 2));
			}
			if (tracer && tracer->interrupt())
				return false;

			int faceCount = facess[n].size();

			trimesh::TriMesh* mesh = new trimesh::TriMesh();
			mesh->faces.reserve(faceCount);
			mesh->vertices.reserve(3 * faceCount);

			int nfacets = facess[n].size();
			for (int nIdx = 0; nIdx < facess[n].size(); nIdx++)
			{
				if (tracer)
				{
					tracer->progress((float)nIdx / (float)nfacets);
				}
				if (tracer && tracer->interrupt())
					return false;

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