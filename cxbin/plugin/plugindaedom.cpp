#include "plugindaedom.h"
//#include "tinyxml/tinyxml.h"
//#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"
#include <map>

#include "trimesh2/TriMesh.h"
#include "mmesh/trimesh/trimeshutil.h"

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
		mesh->vertices.reserve(mesh->vertices.size() + str.getCount() / 3);
		for (size_t j = 0; j < str.getCount(); j += 3) {
			mesh->vertices.push_back(trimesh::point(str.get(j), str.get(j + 1), str.get(j + 2)));
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


	void addFaces(trimesh::TriMesh* mesh, domListOfUInts& vcounts, domListOfUInts& ps, int attrCount, int attrIndex)
	{
#if 1
		int stride = attrCount;
		int acc = 0;
		std::vector<int>faceVertIdx;

		for (int i = 0; i < vcounts.getCount(); i++) {

			faceVertIdx.clear();

			int points = vcounts.get(i);
			for (int j = 0; j < points; j++) {
				faceVertIdx.push_back(ps.get(acc + j * stride + attrIndex));
			}

			for (int p = 2; p < points; p++) {

				trimesh::TriMesh::Face tface;
				tface[0] = faceVertIdx[0];
				tface[1] = faceVertIdx[p - 1];
				tface[2] = faceVertIdx[p];
				mesh->faces.push_back(tface);
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

    bool DaeDomLoader::tryLoad(FILE* f, unsigned fileSize)
    {
        //return IsDaeFile(f, fileSize);
        return true;
    }

    bool DaeDomLoader::load(FILE* f, unsigned fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
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

								for (int j = 0; j < inputLocalOffsetArray.getCount(); ++j)
								{
									domInputLocalOffsetRef& inputLocalOffsetRef = inputLocalOffsetArray[j];
									if (inputLocalOffsetRef->hasAttribute("semantic"))
									{
										std::string semantic = inputLocalOffsetArray[j]->getAttribute(daeString("semantic"));
										if (semantic == "VERTEX")
										{
											step = j;
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

								//pShape->m_iTriangleNum += indexArray->getCount() / 6;
								domFloat_arrayRef vertexArray = sourceArray[0]->getFloat_array();
								pShape->m_iVertexNum = vertexArray->getCount() / 3;

								domListOfFloats& vertexArrayValue = vertexArray->getValue();
								addVertices(addMesh, vertexArrayValue);

							}
							out.push_back(addMesh);
						}


						domPolylist_Array& polylistArray = thisMesh->getPolylist_array();
						size_t polylistCount = polylistArray.getCount();
						if (polylistCount)
						{
							trimesh::TriMesh* addMesh = new trimesh::TriMesh();
							for (int i = 0; i < polylistArray.getCount(); ++i)
							{
								domPolylistRef indexArray = polylistArray[i];
								domInputLocalOffset_Array& inputLocalOffsetArray = indexArray->getInput_array();
								int nums = 0;
								int step = 0;

								for (int j = 0; j < inputLocalOffsetArray.getCount(); ++j)
								{
									domInputLocalOffsetRef& inputLocalOffsetRef = inputLocalOffsetArray[j];
									if (inputLocalOffsetRef->hasAttribute("semantic"))
									{
										std::string semantic = inputLocalOffsetArray[j]->getAttribute(daeString("semantic"));
										if (semantic == "VERTEX")
										{
											step = j;
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

								addFaces(addMesh, listOfUIntsv, listOfUIntsp, nums + 1, step);

								domFloat_arrayRef vertexArray = sourceArray[0]->getFloat_array();

								domListOfFloats& vertexArrayValue = vertexArray->getValue();
								addVertices(addMesh, vertexArrayValue);
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
