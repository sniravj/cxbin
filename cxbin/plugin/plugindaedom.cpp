#include <stdlib.h>
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

#include "imageproc/imageloader.h"

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

    typedef struct DAEeffect
    {
        std::string init_from;
        //std::string format;
        std::string emission;
        std::string ambient;
        std::string diffuse;
        std::string specular;
        std::string emissiontexture;
        std::string ambienttexture;
        std::string diffusetexture;
        std::string speculartexture;

        DAEeffect()
        {
            init_from;
            //std::string format;
            emission="";
            ambient = "";
            diffuse = "";
            specular = "";
            emissiontexture = "";
            ambienttexture = "";
            diffusetexture = "";
            speculartexture = "";
        }
    }daeEffect;

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
        str += pattern;//????????????????
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
                    mesh->faceUVs.push_back(uface);
                }               
			}

			acc += points * stride;
		}
        if (hasUvs)
        {
            mesh->textureIDs.resize(mesh->faceUVs.size(),0);
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

    void getTriangles(domMesh* thisMesh, domSource_Array& sourceArray,std::vector<trimesh::TriMesh*>& out, std::vector<std::vector<std::string>>& mesh2material,const std::string& sourceV, ccglobal::Tracer* tracer)
    {
        domTriangles_Array& triangleArray = thisMesh->getTriangles_array();

        size_t count = sourceArray.getCount();
        size_t triangleCount = triangleArray.getCount();
        if (triangleCount)
        {
            trimesh::TriMesh* addMesh = new trimesh::TriMesh();
            std::vector<std::string> vmaterial;
            for (int i = 0; i < triangleArray.getCount(); ++i)
            {

                domTrianglesRef indexArray = triangleArray[i];
                domListOfUInts indexArrayValue = indexArray->getP()->getValue();

                vmaterial.push_back(indexArray->getMaterial());

                domInputLocalOffset_Array& inputLocalOffsetArray = indexArray->getInput_array();
                int nums = 0;
                int step = 0;
                int stepTEXCOORD = -1;
                //std::string sourceV = "";
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
                            //sourceV = inputLocalOffsetArray[j]->getAttribute(daeString("source"));
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
            }
            out.push_back(addMesh);
            mesh2material.push_back(vmaterial);
        }
    }

    void getPolylist(domMesh* thisMesh, domSource_Array& sourceArray, std::vector<trimesh::TriMesh*>& out, std::vector<std::vector<std::string>>& mesh2material, const std::string& sourceV, ccglobal::Tracer* tracer)
    {
        domPolylist_Array& polylistArray = thisMesh->getPolylist_array();

        size_t count = sourceArray.getCount();
        size_t polylistCount = polylistArray.getCount();
        if (polylistCount)
        {
            trimesh::TriMesh* addMesh = new trimesh::TriMesh();
            std::vector<std::string> vmaterial;
            std::string sourceM = "";
            for (int i = 0; i < polylistArray.getCount(); ++i)
            {
                domPolylistRef indexArray = polylistArray[i];

                vmaterial.push_back(indexArray->getMaterial());

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
            mesh2material.push_back(vmaterial);
        }
    }

    void getGeometry(daeDatabase* data, std::vector<trimesh::TriMesh*>& out, std::vector<std::vector<std::string>>& mesh2material,ccglobal::Tracer* tracer)
    {
        int geometryElementCount = (int)(data->getElementCount(NULL, "geometry", NULL));
        out.reserve(geometryElementCount);
        for (int currentGeometry = 0; currentGeometry < geometryElementCount; currentGeometry++)
        {
            domGeometry* thisGeometry = nullptr;
            data->getElement((daeElement * *)& thisGeometry, currentGeometry, NULL, "geometry");
            if (thisGeometry)
            {
                domMesh* thisMesh = thisGeometry->getMesh();                
                if (thisMesh)
                {
                    std::string sourceV = "";

                    const domVerticesRef verticesArray = thisMesh->getVertices();
                    domInputLocal_Array  domverticesArray = verticesArray->getInput_array();
                    if (domverticesArray.getCount() > 0)
                    {
                        domInputLocalRef indexArray = domverticesArray[0];
                        sourceV = indexArray->getAttribute(daeString("source"));
                    }

                    domSource_Array& sourceArray = thisMesh->getSource_array();
                    size_t count = sourceArray.getCount();
                    if (count > 0)
                    {
                        getTriangles(thisMesh, sourceArray, out, mesh2material, sourceV, tracer);
                        getPolylist(thisMesh, sourceArray, out, mesh2material, sourceV, tracer);
                    }
                }
            }
        }
    }

    std::string findValue(std::map<std::string, std::string> mnewparam, std::string strfind)
    {
        int n = mnewparam.size();
        int count = 0;
        std::map<std::string, std::string>::iterator iter = mnewparam.begin();
        while (1)
        {
            iter = mnewparam.find(strfind);
            if (iter != mnewparam.end())
            {
                strfind = iter->second;
                if (strfind.size())
                {
                    if (strfind[0] == '#')
                        strfind = strfind.substr(1);
                }
            }
            else
            {
                break;
            }
           
            if (count>n)
            {
                break;
            }
            count++;
        }
        return strfind;
    }

    void getImages(daeDatabase* data, std::map<std::string, std::string>& images, ccglobal::Tracer* tracer)
    {
        int imageElementCount = (int)(data->getElementCount(NULL, "image", NULL));
        for (int currentimage = 0; currentimage < imageElementCount; currentimage++)
        {
            domGeometry* thisGeometry = nullptr;
            data->getElement((daeElement * *)& thisGeometry, currentimage, NULL, "image");
            if (thisGeometry)
            {              
                images.insert(std::pair<std::string, std::string>(thisGeometry->getId(), thisGeometry->getChild("init_from")->getCharData()));
                //std::string td = thisGeometry->getId();
                //std::string img = thisGeometry->getChild("init_from")->getCharData();
            }
        }
    }

    void getMaterials(daeDatabase* data, std::map<std::string, std::string>& materials, ccglobal::Tracer* tracer)
    {
        int elementCount = (int)(data->getElementCount(NULL, "material", NULL));
        for (int currentimage = 0; currentimage < elementCount; currentimage++)
        {
            domGeometry* thisGeometry = nullptr;
            data->getElement((daeElement * *)& thisGeometry, currentimage, NULL, "material");
            if (thisGeometry)
            {
                //std::string url = thisGeometry->getId();
                //std::string url = thisGeometry->getChild("instance_effect")->getAttribute(daeString("url"));
                materials.insert(std::pair<std::string, std::string>(thisGeometry->getId(), thisGeometry->getChild("instance_effect")->getAttribute(daeString("url"))));
            }
        }
    }

    void getPhone(domCommon_color_or_texture_typeRef& em, std::map<std::string, std::string>& mnewparams, std::string& color, std::string& texture)
    {
        daeElementRefArray em1 = em->getChildren();
        for (size_t iem = 0; iem < em1.getCount(); iem++)
        {
            std::string elementName = em1[iem]->getElementName();
            if (elementName == "color")
            {
                color = em->getColor()->getCharData();
            }
            else if (elementName == "texture")
            {
                texture = em->getTexture()->getTexture();
                texture = findValue(mnewparams, texture);
            }
        }
    }

    void getEffects(daeDatabase* data, std::map<std::string, daeEffect>& effects, ccglobal::Tracer* tracer)
    {
        daeEffect daeeffect;  

        std::map<std::string, std::string> mnewparams;

        int elementCount = (int)(data->getElementCount(NULL, "effect", NULL));
        for (int currentimage = 0; currentimage < elementCount; currentimage++)
        {
            domEffect* thisGeometry = nullptr;
            data->getElement((daeElement * *)& thisGeometry, currentimage, NULL, "effect");

            if (thisGeometry)
            {
                domProfile_COMMON* thisMesh = (domProfile_COMMON*)thisGeometry->getChild("profile_COMMON");
                std::string id = thisGeometry->getId();   
                std::string value = "";
                domImage_Array ifs = thisGeometry->getImage_array();
                
                domCommon_newparam_type_Array& newparams = thisMesh->getNewparam_array();
                for (size_t i = 0; i < newparams.getCount(); i++)
                {
                    domCommon_newparam_typeRef& newparam = newparams[i];
                    std::string sid = newparam->getSid();

                    daeElementRefArray np1 = newparam->getChildren();
                    for (size_t j = 0; j < np1.getCount(); j++)
                    {
                        for (size_t k = 0; k < np1[j]->getChildren().getCount(); k++)
                        {
                            daeElement* np2 = np1[j]->getChildren()[k];
                            //std::string str = np2->getCharData();
                            std::string elementName = np2->getElementName();
                            if (elementName == "init_from"
                                || elementName == "source")
                            {
                                mnewparams.insert(std::pair<std::string, std::string>(sid, np2->getCharData()));
                            }
                        }
                    }
                }

                domProfile_COMMON::domTechniqueRef technique = thisMesh->getTechnique();
                //domTechnique::dom  domPhongRef e=thisMesh->getPhong();
                if (technique == nullptr) {
                    return;
                }
                
                domProfile_COMMON::domTechnique::domPhongRef pr = technique->getPhong();
                if (pr == nullptr) {
                    return;
                }
                
                domCommon_color_or_texture_typeRef em = pr->getEmission();
                if (em) {
                    getPhone(em, mnewparams, daeeffect.emission, daeeffect.emissiontexture);
                }
                

                
                //domCommon_color_or_texture_typeRef em = pr->getEmission();
                //getPhone(em, mnewparams, daeeffect.emission, daeeffect.emissiontexture);

                domCommon_color_or_texture_typeRef am = pr->getAmbient();
                if (am) {
                    getPhone(am, mnewparams, daeeffect.ambient, daeeffect.ambienttexture);
                }
                //daeeffect.ambient = am->getChild("color")->getCharData();

                domCommon_color_or_texture_typeRef diff = pr->getDiffuse();
                if (diff) {
                    getPhone(diff, mnewparams, daeeffect.diffuse, daeeffect.diffusetexture);
                }
                
                //daeeffect.diffuse = diff->getChild("texture")->getAttribute(daeString("texture"));

                domCommon_color_or_texture_typeRef sp = pr->getSpecular();
                if (sp) {
                    getPhone(sp, mnewparams, daeeffect.specular, daeeffect.speculartexture);
                }
                
                //daeeffect.specular = am->getChild("color")->getCharData();            

                //daeeffect.init_from = findValue(mnewparams, daeeffect.diffuse);

                effects.insert(std::pair<std::string, daeEffect>(id, daeeffect));
            }
        }
    }

    void getVisualScenes(daeDatabase* data, std::map<std::string, std::string>& visualScenes, ccglobal::Tracer* tracer)
    {
        int elementCount = (int)(data->getElementCount(NULL, "visual_scene", NULL));
        for (int currentimage = 0; currentimage < elementCount; currentimage++)
        {
            domVisual_scene* thisGeometry = nullptr;
            data->getElement((daeElement * *)& thisGeometry, currentimage, NULL, "visual_scene");
            if (thisGeometry)
            {
                domNode_Array& nodes = thisGeometry->getNode_array();
                int count = nodes.getCount();
                for (size_t i = 0; i < count; i++)
                {
                    domNodeRef node = nodes[i];
                    domInstance_controller_Array& ic = node->getInstance_controller_array();
                    int count1 = ic.getCount();
                    for (size_t j = 0; j < count1; j++)
                    {
                        domBind_materialRef bm = ic[j]->getBind_material();
                        domInstance_material_Array ims = bm->getTechnique_common()->getInstance_material_array();
                        int count2 = ims.getCount();
                        for (size_t k = 0; k < count2; k++)
                        {
                            domInstance_materialRef im= ims[k];
                            visualScenes.insert(std::pair<std::string, std::string>(im->getSymbol(), im->getTarget().getOriginalURI()));
                        }
                    }
                }
            }
        }
    }

    trimesh::vec getFloatMaterial(std::string& ambient, ccglobal::Tracer* tracer)
    {
        float x, y, z;
        if (std::sscanf(ambient.c_str(), "%f %f %f", &x, &y, &z) != 3) {
            if (tracer)
                tracer->failed("sscanf failed");

            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }
        return trimesh::vec(x, y, z);
    }

    void mergeData(daeDatabase* data,std::vector<trimesh::TriMesh*>& out
        , std::vector<std::vector<std::string>>& mesh2materials
        , std::map<std::string, std::string>& visualScenes
        , std::map<std::string, std::string>& images
        , std::map<std::string, daeEffect>& effects
        , std::map<std::string, std::string>& materials
        , ccglobal::Tracer* tracer)
    {
        for (size_t i = 0; i < out.size(); i++)
        {
            trimesh::TriMesh* mesh = out[i];
            std::vector<std::string> mesh2material = mesh2materials[i];
            std::string m = "";
            for (size_t j = 0; j < mesh2material.size(); j++)
            {
                m = findValue(visualScenes, mesh2material[j]);
                if (!m.empty())
                {
                    m = findValue(materials, m);
                    break;
                }
            }
            
            std::map<std::string, daeEffect>::iterator iter = effects.find(m);
            if (iter != effects.end())
            {
                std::vector<trimesh::Material>& materials = mesh->materials;
                trimesh::Material meterial;
                
                if (!iter->second.ambient.empty())
                    meterial.ambient= getFloatMaterial(iter->second.ambient, tracer);
                if (!iter->second.emission.empty())
                    meterial.emission = getFloatMaterial(iter->second.ambient, tracer);
                if (!iter->second.diffuse.empty())
                    meterial.diffuse = getFloatMaterial(iter->second.ambient, tracer);
                if (!iter->second.specular.empty())
                    meterial.specular = getFloatMaterial(iter->second.ambient, tracer);

                if (!iter->second.ambienttexture.empty())
                {
                    meterial.map_filepaths[trimesh::Material::MapType::AMBIENT] = findValue(images, iter->second.ambienttexture);
                }
                else if (!iter->second.diffusetexture.empty())
                {
                    meterial.map_filepaths[trimesh::Material::MapType::DIFFUSE] = findValue(images, iter->second.diffusetexture);
                }
                else if (!iter->second.speculartexture.empty())
                {                  
                    meterial.map_filepaths[trimesh::Material::MapType::SPECULAR] = findValue(images, iter->second.speculartexture);
                }

                materials.push_back(meterial);
            }
        }
    }

    bool loadMap(trimesh::TriMesh* mesh,const std::string strPath)
    {
        if (mesh == nullptr)
            return false;

        std::vector<trimesh::Material> & materials = mesh->materials;
        for (int type = 0; type < trimesh::Material::TYPE_COUNT; type++)
        {
            imgproc::ImageData* imageData = nullptr;
            int widthMax = 0;
            int heightMax = 0;
            int widthOffset = 0;
            int heightOffset = 0;
            int bytesPerPixel = 4;//FORMAT_RGBA_8888
            std::vector<imgproc::ImageData*> imagedataV(materials.size(), nullptr);
            for (int i = 0; i < materials.size(); i++)
            {
                trimesh::Material& material = materials[i];
                const std::string& fileName = material.map_filepaths[type];
                const std::string& fileName2 = strPath + fileName;
                if (!fileName.empty())
                {
                    imgproc::ImageData* newimagedatetemp = new imgproc::ImageData;
                    newimagedatetemp->format = imgproc::ImageDataFormat::FORMAT_RGBA_8888;
                    imgproc::loadImage_freeImage(*newimagedatetemp, fileName);
                    if (newimagedatetemp->valid() == false && !fileName.empty())
                    {
                        //try full path
                        imgproc::loadImage_freeImage(*newimagedatetemp, fileName2);
                    }

                    if (newimagedatetemp->valid() == false)
                    {
                        imagedataV[i] = nullptr;
                        delete newimagedatetemp;
                    }
                    else
                    {
                        imagedataV[i] = newimagedatetemp;
                        //material.startUV = trimesh::vec2(width0ffset, heightoffset);
                        //material.endUV = trimesh::vec2(width0ffset + imagedataV[i]->width / bytesPerPixel, heightoffset + imagedataV[i]->height);
                        heightOffset += imagedataV[i]->height;
                    }
                }
            }

            if (imagedataV.size() > 0)
            {
                std::vector<std::pair<imgproc::ImageData::point, imgproc::ImageData::point>> imageOffset;
                imageData = imgproc::constructNewFreeImage(imagedataV, imgproc::ImageDataFormat::FORMAT_RGBA_8888, imageOffset);
                if (imageData != nullptr)
                {
                    widthMax = imageData->width / bytesPerPixel;
                    heightMax = imageData->height;
                    for (int i = 0; i < materials.size(); i++)
                    {
                        if (imagedataV[i] != nullptr)
                        {
                            trimesh::Material& material = materials[i];
                            imgproc::ImageData::point startpos = imageOffset[i].first;
                            imgproc::ImageData::point endpos = imageOffset[i].second;
                            material.map_startUVs[type] = trimesh::vec2((float)startpos.x / widthMax, (float)startpos.y / heightMax);
                            material.map_endUVs[type] = trimesh::vec2((float)endpos.x / widthMax, (float)endpos.y / heightMax);
                        }
                    }

                    if (std::max(widthMax, heightMax) > 4096)
                    {
                        float scalevalue = (float)4096.0 / std::max(widthMax, heightMax);
                        imageData = imgproc::scaleFreeImage(imageData, scalevalue, scalevalue);
                    }

                    unsigned char* buffer;
                    unsigned bufferSize;
                    imgproc::writeImage2Mem_freeImage(*imageData, buffer, bufferSize, imgproc::ImageFormat::IMG_FORMAT_PNG);

                    mesh->map_bufferSize[type] = bufferSize;
                    mesh->map_buffers[type] = buffer;
                }
            }

            for (int i = 0; i < imagedataV.size(); i++)
            {
                if (imagedataV[i])
                {
                    delete imagedataV[i];
                    imagedataV[i] = nullptr;
                }
            }
            imagedataV.clear();

            if (imageData)
            {
                delete imageData;
                imageData = nullptr;
            }
        }

        return true;
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

		return true;
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
				tracer->failed("dae dom File open failed");
			return false;
		}

		std::vector<CObject*> ObjectShapes;
		daeDatabase* data = dae.getDatabase();

        //???????��??????
        std::map<std::string, daeEffect> effects;
        getEffects(data, effects, tracer);
        
        //???????????
        std::map<std::string, std::string> materials;
        getMaterials(data,materials,tracer);

        //?????????
        std::map<std::string, std::string> images;
        getImages(data, images, tracer);

        //���Ӳ��ʰ󶨽ӿ�
        std::map<std::string, std::string> visualScenes;
        getVisualScenes(data,visualScenes, tracer);

        //��ȡ���㡢�桢uvs��faceuvs
        std::vector<std::vector<std::string>> mesh2material;
        getGeometry(data,out, mesh2material,tracer);

        //���������Ϣ
        mergeData(data,out,mesh2material,visualScenes,images,effects,materials,tracer);

        std::string materialFileName = "";
        size_t loc = modelPath.find_last_of("/") + 1;
        if (loc != std::string::npos) {
            materialFileName = modelPath.substr(0, loc);
        }
        for (size_t i = 0; i < out.size(); i++)
        {
            if (!loadMap(out[i], materialFileName))
            {
                return !success;
            }
        }

        //just for test
        //std::vector<std::shared_ptr<AssociateFileInfo>> out1;
        //associateFileList(f, tracer, modelPath, out1);

        if (tracer)
        {
            tracer->success();
        }
        return !success;
    }

    void DaeDomLoader::associateFileList(FILE* f, ccglobal::Tracer* tracer, const std::string& filePath, std::vector<std::shared_ptr<AssociateFileInfo>>& out)
    {
        DAE dae;
        domCOLLADA* root = (domCOLLADA*)dae.open(filePath);
        if (!root)
        {
            if (tracer)
                tracer->failed("dae dom File open failed");
            return ;
        }

        std::string imagesFileName = "";
        size_t loc = filePath.find_last_of("/") + 1;
        if (loc != std::string::npos) {
            imagesFileName = filePath.substr(0, loc);
        }

        std::vector<CObject*> ObjectShapes;
        daeDatabase* data = dae.getDatabase();

        std::map<std::string, std::string> images;
        getImages(data, images, tracer);

        std::map<std::string, std::string>::iterator iter = images.begin();
        while (iter != images.end())
        {

            std::shared_ptr<AssociateFileInfo>associateInfor(new AssociateFileInfo());
            if (boost::filesystem::exists(iter->second))
            {
                associateInfor->code = CXBinLoaderCode::no_error;
                associateInfor->path = iter->second;
                out.push_back(associateInfor);
            }
            else {

                const std::string strFullPath = imagesFileName + iter->second;
                if (boost::filesystem::exists(strFullPath))
                {
                    associateInfor->code = CXBinLoaderCode::no_error;
                    associateInfor->path = strFullPath;
                    out.push_back(associateInfor);
                }
                else
                {
                    associateInfor->code = CXBinLoaderCode::file_not_exist;
                    associateInfor->path = strFullPath;
                    out.push_back(associateInfor);
                }
            }

            iter++;
        }
    }
}
