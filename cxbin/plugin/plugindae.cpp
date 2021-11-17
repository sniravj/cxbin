#include "plugindae.h"
#include "tinyxml/tinyxml.h"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
	DaeLoader::DaeLoader()
	{

	}

	DaeLoader::~DaeLoader()
	{

	}

	const TiXmlNode* findNode(const TiXmlNode* node, std::string strNode)
	{
		if (node == nullptr)
			return nullptr;
		for (const TiXmlNode* sub_node = node->FirstChild(); sub_node; sub_node = sub_node->NextSibling())
		{
			if (sub_node->Type() == TiXmlNode::TINYXML_ELEMENT)
			{
				const TiXmlElement* sub_element = (const TiXmlElement*)sub_node;
				if (!strcmp(sub_element->Value(), strNode.c_str()))
				{
					return  sub_node;
				}
			}
		}
		return nullptr;
	}

	bool IsDaeFile(FILE* f, unsigned int fileSize) 
	{
		TiXmlDocument doc;
		if (!doc.LoadFile(f))
		{
			return false;
		}

		const TiXmlElement* root = doc.RootElement();

		if (nullptr == findNode(root, "library_geometries"))
		{
			return false;
		}

		return true;
	}

	std::string DaeLoader::expectExtension()
	{
		return "dae";
	}

	bool DaeLoader::tryLoad(FILE* f, unsigned fileSize)
	{
		return IsDaeFile(f, fileSize);
	}

	std::vector<float> split(std::string str, std::string pattern)
	{
		std::string::size_type pos;
		std::vector<float> result;
		str += pattern;//扩展字符串以方便操作  
		int size = str.size();
		for (int i = 0; i < size; i++)
		{
			pos = str.find(pattern, i);
			if (pos < size)
			{
				std::string s = str.substr(i, pos - i);
				result.push_back(std::stof(s));
				i = pos + pattern.size() - 1;
			}
		}
		return result;
	}

	std::vector<int> splitInt(std::string str, std::string pattern)
	{
		std::string::size_type pos;
		std::vector<int> result;
		str += pattern;//扩展字符串以方便操作  
		int size = str.size();
		for (int i = 0; i < size; i++)
		{
			pos = str.find(pattern, i);
			if (pos < size)
			{
				std::string s = str.substr(i, pos - i);
				result.push_back(std::stoi(s));
				i = pos + pattern.size() - 1;
			}
		}
		return result;
	}

	void findNodes(const TiXmlNode* node, std::vector<const TiXmlNode*>& out, std::string strNode)
	{
		if (node == nullptr)
			return;
		for (const TiXmlNode* sub_node = node->FirstChild(); sub_node; sub_node = sub_node->NextSibling())
		{
			if (sub_node->Type() == TiXmlNode::TINYXML_ELEMENT)
			{
				const TiXmlElement* sub_element = (const TiXmlElement*)sub_node;
				if (!strcmp(sub_element->Value(), strNode.c_str()))
				{
					out.push_back(sub_node);
				}
			}
		}
		return;
	}

	void addVertices(trimesh::TriMesh* mesh, const std::string& str, int& count)
	{
		//218.608183 65.943044 844.069388 155.932865
		mesh->vertices.reserve(count);
		std::vector<float> vertices = split(str, " ");
		for (int i = 0; i < count; i = i + 3)
		{
			mesh->vertices.push_back(trimesh::point(vertices[i], vertices[i + 1], vertices[i + 2]));
		}
	}

	void addNormals(trimesh::TriMesh* mesh, const std::string& str, const int& count)
	{
		//218.608183 65.943044 844.069388 155.932865
		mesh->normals.reserve(count);
		std::vector<float> normals = split(str, " ");
		for (int i = 0; i < count; i = i + 3)
		{
			mesh->normals.push_back(trimesh::point(normals[i], normals[i + 1], normals[i + 2]));
		}
	}

	void addFaces(trimesh::TriMesh* mesh, const std::string& str, const int& count, const int& vffset, const int& nffset)
	{
		std::vector<int> p = splitInt(str, " ");
		for (int i = 0; i < p.size() && (i + vffset + count * 2) < p.size(); i = i + count * 3)
		{
			trimesh::TriMesh::Face tface;
			tface[0] = p[i + vffset];
			tface[1] = p[i + vffset + count];
			tface[2] = p[i + vffset + count*2];
			mesh->faces.push_back(tface);
		}
	}

	void addFaces(trimesh::TriMesh* mesh, const std::string& vcount, const std::string& p, const int& vffset, const int& nffset)
	{
		std::vector<int> vcounts = splitInt(vcount, " ");
		std::vector<int> ps = splitInt(p, " ");

		std::vector<int> vertex;
		for (int i = 0; i < ps.size(); i = i + 3)
		{
			vertex.push_back(ps[i]);
		}

		int index = 0;
		for (int i = 0; i < vertex.size();)
		{
			trimesh::TriMesh::Face tface;
			for (int j = 1; j < vcounts[index] - 1; j++)
			{
				trimesh::TriMesh::Face tface;
				tface[0] = vertex[i];
				tface[1] = vertex[i + j];
				tface[2] = vertex[i + j + 1];

				mesh->faces.push_back(tface);
			}

			i += vcounts[index];
			index++;
		}
	}

	bool DaeLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		bool success = false;

		TiXmlDocument doc;
		if (!doc.LoadFile(f))
		{
			if (tracer)
			{
				tracer->failed("Xml File open failed");
			}
			return success;
		}

		//root
		const TiXmlElement* root = doc.RootElement();
		const TiXmlNode* _geometries = findNode(root, "library_geometries");
		std::vector<const TiXmlNode*> geometries;
		findNodes(_geometries, geometries, "geometry");

		for (const TiXmlNode* geometry : geometries)
		{
			if (tracer && tracer->interrupt())
				return false;

			std::vector<const TiXmlNode*> meshes;
			findNodes(geometry, meshes, "mesh");

			int size = meshes.size();
			int curSize = 0;
			for (const TiXmlNode* mesh : meshes)
			{
				int positioncount = 0;
				int normalcount = 0;
				const char* position = nullptr;
				const char* normal = nullptr;

				std::vector<const TiXmlNode*> sources;
				findNodes(mesh, sources, "source");
				for (const TiXmlNode* source : sources)
				{
					const TiXmlNode* _floatarry = findNode(source, "float_array");
					const TiXmlElement* floatarry = (const TiXmlElement*)_floatarry;

					std::string strid = floatarry->Attribute("id");
					if (strid.find("position") != std::string::npos)
					{
						floatarry->Attribute("count", &positioncount);
						position = floatarry->GetText();
						//addVertices(mesh, sub_val->GetText(), icount);
					}
					else if (strid.find("normal") != std::string::npos)
					{
						floatarry->Attribute("count", &normalcount);
						normal = floatarry->GetText();
						//addNormals(mesh, sub_val->GetText(), icount);
					}
				}
				if (tracer)
				{
					tracer->progress((float)curSize++ / (float)size*3);
				}

				std::vector<const TiXmlNode*> polylists;
				findNodes(mesh, polylists, "polylist");
				for (const TiXmlNode* polylist : polylists)
				{
					int vertexOffset = 0;
					int normalOffset = 0;

					trimesh::TriMesh* mesh = new trimesh::TriMesh();
					if (position != nullptr)
						addVertices(mesh, position, positioncount);
					if (normal != nullptr)
						addNormals(mesh, normal, normalcount);

					std::vector<const TiXmlNode*> inputs;
					findNodes(polylist, inputs, "input");
					for (const TiXmlNode* node : inputs)
					{
						const TiXmlElement* sub_ele = (const TiXmlElement*)node;
						if (!strcmp(sub_ele->Attribute("semantic"), "VERTEX"))
						{
							sub_ele->Attribute("offset", &vertexOffset);
						}
						if (!strcmp(sub_ele->Attribute("semantic"), "NORMAL"))
						{
							sub_ele->Attribute("offset", &normalOffset);
						}
					}

					const TiXmlNode* _vcount = findNode(polylist, "vcount");
					const TiXmlElement* vcount = (const TiXmlElement*)_vcount;
					const TiXmlNode* _p = findNode(polylist, "p");
					const TiXmlElement* p = (const TiXmlElement*)_p;
					addFaces(mesh, vcount->GetText(), p->GetText(), vertexOffset, normalOffset);
					out.push_back(mesh);
				}
				if (tracer)
				{
					tracer->progress((float)curSize++ / (float)size * 3);
				}

				std::vector<const TiXmlNode*> triangles;
				findNodes(mesh, triangles, "triangles");
				for (const TiXmlNode* triangle : triangles)
				{
					int vertexOffset = 0;
					int normalOffset = 0;

					trimesh::TriMesh* mesh = new trimesh::TriMesh();
					if (position != nullptr)
						addVertices(mesh, position, positioncount);
					if (normal != nullptr)
						addNormals(mesh, normal, normalcount);

					std::vector<const TiXmlNode*> inputs;
					findNodes(triangle, inputs, "input");
					for (const TiXmlNode* node : inputs)
					{
						const TiXmlElement* sub_ele = (const TiXmlElement*)node;
						if (!strcmp(sub_ele->Attribute("semantic"), "VERTEX"))
						{
							sub_ele->Attribute("offset", &vertexOffset);
						}
						if (!strcmp(sub_ele->Attribute("semantic"), "NORMAL"))
						{
							sub_ele->Attribute("offset", &normalOffset);
						}
					}

					const TiXmlNode* _p = findNode(triangle, "p");
					const TiXmlElement* p = (const TiXmlElement*)_p;
					if (inputs.size() > 0 )
					{
						addFaces(mesh, p->GetText(), inputs.size(), vertexOffset, normalOffset);
					}
					out.push_back(mesh);
				}
				if (tracer)
				{
					tracer->progress((float)curSize++ / (float)size * 3);
				}
			}
		}
		if (tracer)
		{
			tracer->success();
		}
		return !success;
	}
}