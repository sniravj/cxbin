#include "plugindaedom.h"
#include "tinyxml/tinyxml.h"
//#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"
#include <map>

#include "trimesh2/TriMesh.h"

#include <dae.h>
#include <dom/domConstants.h>
#include <dom/domCOLLADA.h>
#include <dom/domProfile_COMMON.h>
#include <dae/daeSIDResolver.h>
#include <dom/domInstance_controller.h>
#include <dae/domAny.h>
#include <dae/daeErrorHandler.h>
#include <dae/daeUtils.h>
#include <dom/domFx_surface_init_from_common.h>
#include <modules/stdErrPlugin.h>
#include <dom/domEllipsoid.h>
#include <dom/domInputGlobal.h>
#include <dom/domAsset.h>
#include <dom/domGlsl_surface_type.h>
#include <dom/domAny.h>
#include <dom/domPolylist.h>

using namespace ColladaDOM141;

namespace cxbin
{
	struct CObject
	{
		std::string m_sName;
		size_t m_iVertexNum;
		size_t m_iNormalNum;
		float* m_pVertices;
		float* m_pNormals;

		size_t m_iTriangleNum;
	};

    DaeDomLoader::DaeDomLoader()
    {

    }

    DaeDomLoader::~DaeDomLoader()
    {

    }


    std::vector<float>  DaeDomLoader::split(std::string str, std::string pattern)
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

	void addVertices(trimesh::TriMesh* mesh, const domListOfFloats& str)
	{
		////218.608183 65.943044 844.069388 155.932865
        if (str.getCount() % 3 != 0)
        {
            return;
        }

		mesh->vertices.reserve(mesh->vertices.size() + str.getCount() / 3);
		for (size_t j = 0; j < str.getCount(); j += 3) {
			mesh->vertices.push_back(trimesh::point(str.get(j), str.get(j + 1), str.get(j + 2)));
		}
	}

    void addUvs(trimesh::TriMesh* mesh, const domListOfFloats& str)
    {
        ////218.608183 65.943044 844.069388 155.932865
        if (str.getCount()%2 != 0)
        {
            return;
        }

        mesh->UVs.reserve(mesh->vertices.size() + str.getCount() / 2);
        for (size_t j = 0; j < str.getCount(); j += 2) {
            mesh->UVs.push_back(trimesh::vec2(str.get(j), str.get(j + 1)));
        }
    }

	void addFaces(trimesh::TriMesh* mesh, const domListOfUInts& str, int nums, int step)
	{
		mesh->faces.reserve(mesh->faces.size() + str.getCount() / 3);
		for (int i = 0; i < str.getCount() && (i + step + nums * 2) < str.getCount(); i = i + nums * 3)
		{
			trimesh::TriMesh::Face tface;
			tface[0] = str.get(i + step);
			tface[1] = str.get(i + step + nums);
			tface[2] = str.get(i + step + nums * 2);
			mesh->faces.push_back(tface);
		}
	}


	void addFaces(trimesh::TriMesh* mesh, domListOfUInts& vcounts, domListOfUInts& ps, int attrCount, int attrIndex, int faceUvs)
	{
#if 1
		int stride = attrCount;
        int psize = ps.getCount();
		int acc = 0;
		std::vector<int>faceVertIdx;
        std::vector<int>faceUvsIdx;
        bool hasUvs = faceUvs >= 0 ? true : false;
        int count = vcounts.getCount();
		for (int i = 0; i < count; i++) {

			faceVertIdx.clear();
            faceUvsIdx.clear();
			int points = vcounts.get(i);
			for (int j = 0; j < points; j++) {
                
                if (psize > acc + j * stride + attrIndex)
				    faceVertIdx.push_back(ps.get(acc + j * stride + attrIndex));
                
                if (hasUvs)
                {
                    if (psize > acc + j * stride + faceUvs)
                        faceUvsIdx.push_back(ps.get(acc + j * stride + faceUvs));
                }
			}

			for (int p = 2; p < points; p++) {

				trimesh::TriMesh::Face tface;
				tface[0] = faceVertIdx[0];
				tface[1] = faceVertIdx[p - 1];
				tface[2] = faceVertIdx[p];
                mesh->faces.push_back(tface);

                if (hasUvs)
                {
                    trimesh::TriMesh::Face uface;
                    uface[0] = faceUvsIdx[0];
                    uface[1] = faceUvsIdx[p - 1];
                    uface[2] = faceUvsIdx[p];
                    mesh->faceUVs.push_back(tface);
                }               
			}

			acc += points * stride;
		}
#else
		std::vector<int> vertex;
		for (int i = 0; i < ps.size(); i = i + 4)
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
#endif

	}



    std::string DaeDomLoader::expectExtension()
    {
        return "dae";
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

	bool IsDaeFile(FILE* f, size_t fileSize)
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

		//return true;
	}

    bool DaeDomLoader::tryLoad(FILE* f, size_t fileSize)
    {
        return IsDaeFile(f, fileSize);
    }

    bool DaeDomLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
    {
        bool success = false;


		DAE dae;
		domCOLLADA* root = (domCOLLADA*)dae.open(modelPath);
		if (!root)
		{

			if (tracer)
			{
				tracer->failed("dae dom File open failed");
			}
			return false;
		}

		std::vector<CObject*> ObjectShapes;
		daeDatabase* data = dae.getDatabase();

        int imageElementCount = (int)(data->getElementCount(NULL, "image", NULL));
        for (int currentimage = 0; currentimage < imageElementCount; currentimage++)
        {
            domGeometry* thisGeometry = nullptr;
            data->getElement((daeElement * *)& thisGeometry, currentimage, NULL, "image");
            if (thisGeometry)
            {
                std::string td = thisGeometry->getId();
                std::string img = thisGeometry->getChild("init_from")->getCharData();
            }
        }

		int geometryElementCount = (int)(data->getElementCount(NULL, "geometry", NULL));
		out.reserve(geometryElementCount);
		for (int currentGeometry = 0; currentGeometry < geometryElementCount; currentGeometry++)
		{
			domGeometry* thisGeometry = nullptr;
			data->getElement((daeElement**)&thisGeometry, currentGeometry, NULL, "geometry");
			if (thisGeometry)
			{
				domMesh* thisMesh = thisGeometry->getMesh();
				if (thisMesh)
				{
					//std::cout << thisMesh->getID() << std::endl;
					CObject* pShape = new CObject;

					domSource_Array& sourceArray = thisMesh->getSource_array();
					size_t count = sourceArray.getCount();
					if (count > 0)
					{
						domTriangles_Array& triangleArray = thisMesh->getTriangles_array();
						size_t triangleCount = triangleArray.getCount();
						if (triangleCount)
						{
							trimesh::TriMesh* addMesh = new trimesh::TriMesh();
							for (int i = 0; i < triangleArray.getCount(); ++i)
							{

								domTrianglesRef indexArray = triangleArray[i];
								domListOfUInts indexArrayValue = indexArray->getP()->getValue();

								domInputLocalOffset_Array& inputLocalOffsetArray = indexArray->getInput_array();
								int nums = 0;
								int step = 0;
                                int stepTEXCOORD = -1;
                                std::string sourceV = "";
                                std::string sourceM = "";

								for (int j = 0; j < inputLocalOffsetArray.getCount(); ++j)
								{
									domInputLocalOffsetRef& inputLocalOffsetRef = inputLocalOffsetArray[j];
									if (inputLocalOffsetRef->hasAttribute("semantic"))
									{
										std::string semantic = inputLocalOffsetArray[j]->getAttribute(daeString("semantic"));
										if (semantic == "VERTEX")
										{
											step = j;
                                            sourceV = inputLocalOffsetArray[j]->getAttribute(daeString("source"));
										}
                                        else if (semantic == "TEXCOORD")
                                        {
                                            stepTEXCOORD = j;
                                            sourceM = inputLocalOffsetArray[j]->getAttribute(daeString("source"));
                                        }
									}
									if (inputLocalOffsetRef->hasAttribute("offset"))
									{
										std::string strValue;
										inputLocalOffsetArray[j]->getAttribute(daeString("offset"), strValue);
										int intValue = std::stoi(strValue);
										nums = std::max(nums, intValue);
									}
								}

								addFaces(addMesh, indexArrayValue, nums + 1, step);

                                for (size_t i = 0; i < count; i++)
                                {
                                    domSourceRef& sourceIndex = sourceArray[i];

                                    std::string str = sourceIndex->getId();

                                    if (sourceV.find(str) != std::string::npos)
                                    {
                                        domFloat_arrayRef vertexArray = sourceIndex->getFloat_array();

                                        domListOfFloats& vertexArrayValue = vertexArray->getValue();
                                        addVertices(addMesh, vertexArrayValue);
                                    }
                                    else if (sourceM.find(str) != std::string::npos)
                                    {
                                        domFloat_arrayRef vertexArray = sourceIndex->getFloat_array();

                                        domListOfFloats& vertexArrayValue = vertexArray->getValue();
                                        addUvs(addMesh, vertexArrayValue);
                                    }
                                }

                                //if (addMesh->vertices.empty())
                                //{
                                //    //pShape->m_iTriangleNum += indexArray->getCount() / 6;
                                //    domFloat_arrayRef vertexArray = sourceArray[0]->getFloat_array();
                                //    pShape->m_iVertexNum = vertexArray->getCount() / 3;

                                //    domListOfFloats& vertexArrayValue = vertexArray->getValue();
                                //    addVertices(addMesh, vertexArrayValue);
                                //}
							}
							out.push_back(addMesh);
						}


						domPolylist_Array& polylistArray = thisMesh->getPolylist_array();
						size_t polylistCount = polylistArray.getCount();
						if (polylistCount)
						{
							trimesh::TriMesh* addMesh = new trimesh::TriMesh();
                            std::string sourceV = "";
                            std::string sourceM = "";
							for (int i = 0; i < polylistArray.getCount(); ++i)
							{
								domPolylistRef indexArray = polylistArray[i];
								domInputLocalOffset_Array& inputLocalOffsetArray = indexArray->getInput_array();
								int nums = 0;
								int step = 0;
                                int stepTEXCOORD = -1;

								for (int j = 0; j < inputLocalOffsetArray.getCount(); ++j)
								{
									domInputLocalOffsetRef& inputLocalOffsetRef = inputLocalOffsetArray[j];
									if (inputLocalOffsetRef->hasAttribute("semantic"))
									{
										std::string semantic = inputLocalOffsetArray[j]->getAttribute(daeString("semantic"));
										if (semantic == "VERTEX")
										{
											step = j;
                                            sourceV = inputLocalOffsetArray[j]->getAttribute(daeString("source"));
                                        }
                                        else if (semantic == "TEXCOORD")
                                        {
                                            stepTEXCOORD = j;
                                            sourceM = inputLocalOffsetArray[j]->getAttribute(daeString("source"));
                                        }
									}
									if (inputLocalOffsetRef->hasAttribute("offset"))
									{
										std::string strValue;
										inputLocalOffsetArray[j]->getAttribute(daeString("offset"), strValue);
										int intValue = std::stoi(strValue);
										nums = std::max(nums, intValue);
									}
								}

								ColladaDOM141::domPolylist::domVcountRef vcountRef = indexArray->getVcount();
								domListOfUInts& listOfUIntsv = vcountRef->getValue();
								domPRef pRef = indexArray->getP();
								domListOfUInts& listOfUIntsp = pRef->getValue();

								addFaces(addMesh, listOfUIntsv, listOfUIntsp, nums + 1, step, stepTEXCOORD);

                                //if (addMesh->vertices.empty())
                                //{
                                //    domFloat_arrayRef vertexArray = sourceArray[0]->getFloat_array();

                                //    domListOfFloats& vertexArrayValue = vertexArray->getValue();
                                //    addVertices(addMesh, vertexArrayValue);
                                //}
							}

                            for (size_t i = 0; i < count; i++)
                            {
                                domSourceRef& sourceIndex = sourceArray[i];

                                std::string str = sourceIndex->getId();

                                if (sourceV.find(str) != std::string::npos)
                                {
                                    domFloat_arrayRef vertexArray = sourceIndex->getFloat_array();

                                    domListOfFloats& vertexArrayValue = vertexArray->getValue();
                                    addVertices(addMesh, vertexArrayValue);
                                }
                                else if (sourceM.find(str) != std::string::npos)
                                {
                                    domFloat_arrayRef vertexArray = sourceIndex->getFloat_array();

                                    domListOfFloats& vertexArrayValue = vertexArray->getValue();
                                    addUvs(addMesh, vertexArrayValue);
                                }
                            }
							out.push_back(addMesh);
						}

					}
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
