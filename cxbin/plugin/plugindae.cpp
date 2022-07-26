#include "plugindae.h"
#include "tinyxml/tinyxml.h"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"
#include <map>

namespace cxbin
{
    DaeLoader::DaeLoader()
    {

    }

    DaeLoader::~DaeLoader()
    {

    }

    typedef struct SOURCEGROUP
    {
        std::string position;
        int count;
        int stride;
    }sourceGroup;

    typedef struct DaeImage {
        std::string id;
        std::string name;
    }DaeImage;

    typedef struct Instancegeometry {
        std::string id;
        std::string url;
        std::string symbol;
        std::string target;
    }Instancegeometry;

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

        return true;
    }

    std::string DaeLoader::expectExtension()
    {
        return "dae";
    }

    bool DaeLoader::tryLoad(FILE* f, size_t fileSize)
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

    const char* safeAttribute(const TiXmlElement *from, const char* name )
    {
        if (from == nullptr) {
            return "";
        }
        const char* v = from->Attribute(name);
        return v ? v : "";
    }
    
    const char* safeAttribute(const TiXmlElement *from, const std::string& name )
    {
        if (from == nullptr) {
            return "";
        }
        
        const std::string* v = from->Attribute(name);
        if (v) {
            return v->c_str();
        }
        
        return "";
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

    const TiXmlNode* findNodesWithAttribule(const TiXmlNode* node, const std::string& attribule, const std::string& attribuleValue)
    {
        if (node == nullptr)
            return nullptr;
        for (const TiXmlNode* sub_node = node->FirstChild(); sub_node; sub_node = sub_node->NextSibling())
        {
            if (sub_node->Type() == TiXmlNode::TINYXML_ELEMENT)
            {
                const TiXmlElement* sub_element = (const TiXmlElement*)sub_node;
                const std::string *v = sub_element->Attribute(attribule);
                if (v && *v == attribuleValue) {
                    return sub_element;
                }
            }
        }
        return nullptr;
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

    void addFaces(std::vector<int>& vcounts, std::vector<int>& ps, int attrCount, int attrIndex, std::vector<trimesh::TriMesh::Face>& outFaces)
    {
#if 1
        int stride = attrCount;
        int acc = 0;
        std::vector<int>faceVertIdx;
        
        for (int i = 0; i < vcounts.size(); i++) {
            
            faceVertIdx.clear();
            
            int points = vcounts.at(i);
            for (int j = 0; j < points; j++) {
                faceVertIdx.push_back(ps.at(acc + j*stride + attrIndex));
            }
            
            for (int p = 2; p < points; p++) {
                
                trimesh::TriMesh::Face tface;
                tface[0] = faceVertIdx[0];
                tface[1] = faceVertIdx[p-1];
                tface[2] = faceVertIdx[p];
                outFaces.push_back(tface);
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

    void addFacesOfTriangle(int count, std::vector<int>& ps, int attrCount, int attrIndex, std::vector<trimesh::TriMesh::Face>& outFaces)
    {
        int stride = attrCount;
        int acc = 0;
        int points = 3;
        for (int i = 0; i < count; i++) {

            trimesh::TriMesh::Face tface;

            for (int j = 0; j < points; j ++) {
                tface[j] = ps[acc + stride * j + attrIndex];
            }
            
            outFaces.push_back(tface);
            
            acc += points * stride;
        }
    }

    void parsePoints(std::string& input, int count, int stride, std::vector<trimesh::point>& outPoints)
    {
        std::vector<float> vertices = split(input, " ");
        for (int i = 0; i < count; i += stride)
        {
            outPoints.push_back(trimesh::point(vertices[i], vertices[i + 1], vertices[i + 2]));
        }
        vertices.clear();
    }

    void parseUVs(std::string& input, int count, int stride, std::vector<trimesh::vec2>& outUVs)
    {
        std::vector<float> vertices = split(input, " ");
        
        if (vertices.size() != count) {
            return;
        }
        
        if (stride == 0) {
            return;
        }
        
        int loopNum = count / stride;
        
        for (int i = 0; i < loopNum; i++)
        {
            outUVs.push_back(trimesh::vec2(vertices[i*stride], vertices[i*stride + 1]));
        }
        vertices.clear();
    }

    void parseImages(const TiXmlNode* node, std::vector<DaeImage>& outImages)
    {
        if (node) {
            std::vector<const TiXmlNode*> images;
            findNodes(node, images, "image");
            for (const TiXmlNode* image : images) {
                
                std::string id = safeAttribute((TiXmlElement*)image, "id");
                const TiXmlElement* ifm = (const TiXmlElement*)findNode(image, "init_from");
                if (ifm) {
                    std::string name = ifm->GetText();
                    DaeImage i;
                    i.id = id;
                    i.name = name;
                    outImages.push_back(i);
                }
            }
        }
    }

    void parsePolylists(std::vector<const TiXmlNode*>& polylists, std::map<std::string, sourceGroup>& sourceGroupMap, std::vector<trimesh::TriMesh*>& outMesh, std::vector<std::string>& outMeshMaterials)
    {
        for (const TiXmlNode* polylist : polylists)
        {
            int vertexOffset = 0;
            int uvOffset = 0;
            int maxOffset = 0;
            trimesh::TriMesh* mesh = new trimesh::TriMesh();

            std::vector<const TiXmlNode*> inputs;
            findNodes(polylist, inputs, "input");
            for (const TiXmlNode* node : inputs)
            {
                const TiXmlElement* sub_ele = (const TiXmlElement*)node;
                int tmpOffset = 0;
                sub_ele->Attribute("offset", &tmpOffset);
                maxOffset = std::max(maxOffset, tmpOffset);
                
                const char* semantic = safeAttribute(sub_ele, "semantic");
                
                if (!strcmp(semantic, "VERTEX"))
                {
                    vertexOffset = tmpOffset;
                    std::string sourceid = safeAttribute(sub_ele, "source");
                    if (sourceid.length() > 1)
                    {
                        if (sourceid.at(0) == '#')
                        {
                            sourceid = sourceid.substr(1, sourceid.length() - 1);
                            auto iter = sourceGroupMap.find(sourceid);
                            if (iter != sourceGroupMap.end())
                            {
                                parsePoints(iter->second.position, iter->second.count, 3, mesh->vertices);
                            }
                        }
                    }

                }
                
                if (!strcmp(semantic, "TEXCOORD"))
                {
                    uvOffset = tmpOffset;
                    std::string sourceid = safeAttribute(sub_ele, "source");
                    if (sourceid.length() > 1)
                    {
                        if (sourceid.at(0) == '#')
                        {
                            sourceid = sourceid.substr(1, sourceid.length() - 1);
                            auto iter = sourceGroupMap.find(sourceid);
                            if (iter != sourceGroupMap.end())
                            {
                                parseUVs(iter->second.position, iter->second.count, iter->second.stride, mesh->UVs);
                            }
                        }
                    }
                }
            }

            const TiXmlElement* vcount = (const TiXmlElement*)findNode(polylist, "vcount");
            const TiXmlElement* p = (const TiXmlElement*)findNode(polylist, "p");
            if (vcount == nullptr || p == nullptr) {
                continue;
            }
            const char* vcountdata = vcount->GetText();
            const char* pdata = p->GetText();
            if (vcountdata != nullptr && pdata != nullptr)
            {
                std::vector<int> vcounts = splitInt(vcountdata, " ");
                std::vector<int> ps = splitInt(pdata, " ");
                
                if (mesh->vertices.size()) {
                    addFaces(vcounts, ps, (maxOffset + 1), vertexOffset, mesh->faces);
                }
                
                if (mesh->UVs.size()) {
                    addFaces(vcounts, ps, (maxOffset + 1), uvOffset, mesh->faceUVs);
                }
            }

            outMesh.push_back(mesh);
            
            std::string mat = safeAttribute((const TiXmlElement *)polylist, "material");
            outMeshMaterials.push_back(mat);
        }
    }


    void parseTriangles(std::vector<const TiXmlNode*>& triangles, std::map<std::string, sourceGroup>& sourceGroupMap, std::vector<trimesh::TriMesh*>& outMesh, std::vector<std::string>& outMeshMaterials)
    {
        for (const TiXmlNode* triangle : triangles)
        {
            int vertexOffset = 0;
            int uvOffset = 0;
            int maxOffset = 0;
            int count = 0;
            ((const TiXmlElement *)triangle)->Attribute("count", &count);
            
            trimesh::TriMesh* mesh = new trimesh::TriMesh();

            std::vector<const TiXmlNode*> inputs;
            findNodes(triangle, inputs, "input");
            
            for (int idx = 0; idx < inputs.size(); idx++)
            {
                const TiXmlNode* node = inputs[idx];
                const TiXmlElement* sub_ele = (const TiXmlElement*)node;
                int tmpOffset = 0;
                sub_ele->Attribute("offset", &tmpOffset);
                maxOffset = std::max(maxOffset, tmpOffset);
                
                const char *semantic = safeAttribute(sub_ele, "semantic");
                
                if (!strcmp(semantic, "VERTEX"))
                {
                    //offset
                    vertexOffset = tmpOffset;
                    std::string sourceid = safeAttribute(sub_ele, "source");
                    if (sourceid.length() > 1)
                    {
                        if (sourceid.at(0) == '#')
                        {
                            sourceid = sourceid.substr(1, sourceid.length() - 1);
                            auto iter = sourceGroupMap.find(sourceid);
                            if (iter != sourceGroupMap.end())
                            {
                                parsePoints(iter->second.position, iter->second.count, 3, mesh->vertices);
                            }
                        }
                    }
                    continue;
                }
                
                if (!strcmp(semantic, "TEXCOORD"))
                {
                    uvOffset = tmpOffset;
                    std::string sourceid = safeAttribute(sub_ele, "source");
                    if (sourceid.length() > 1)
                    {
                        if (sourceid.at(0) == '#')
                        {
                            sourceid = sourceid.substr(1, sourceid.length() - 1);
                            auto iter = sourceGroupMap.find(sourceid);
                            if (iter != sourceGroupMap.end())
                            {
                                parseUVs(iter->second.position, iter->second.count, iter->second.stride, mesh->UVs);
                            }
                        }
                    }
                    continue;
                }
                
                if (!strcmp(semantic, "NORMAL")) {
                    
                    continue;
                }
            }
            
            const TiXmlElement* p = (const TiXmlElement*)findNode(triangle, "p");
            if (inputs.size() > 0 && p != nullptr)
            {
                const char* data = p->GetText();
                if (data != nullptr)
                {
                    
                    std::vector<int> ps = splitInt(data, " ");
                    
                    if (mesh->vertices.size()) {
                        addFacesOfTriangle(count, ps, (maxOffset + 1), vertexOffset, mesh->faces);
                    }
                    if (mesh->UVs.size()) {
                        addFacesOfTriangle(count, ps, (maxOffset + 1), uvOffset, mesh->faceUVs);
                    }
                    
                    
                    outMesh.push_back(mesh);
                    std::string mat = safeAttribute((const TiXmlElement *)triangle, "material");
                    outMeshMaterials.push_back(mat);
                }
            }
        }
    }

    void parsePolygons(std::vector<const TiXmlNode*>& polygons, std::map<std::string, sourceGroup>& sourceGroupMap, std::vector<trimesh::TriMesh*>& outMesh, std::vector<std::string>& outMeshMaterials)
    {
        for (const TiXmlNode* polygon : polygons)
        {
            //polygon_with_holes.dae
            //todo:
        }
    }

    void parseMaterial(const TiXmlElement* root, const std::string name, trimesh::Material &mate)
    {
        if (root == nullptr) {
            return;
        }
        
        std::string id = "id";
        std::string materURL;
        const TiXmlNode *matout;
        const TiXmlNode* materialsNode = findNode(root, "library_materials");
        if (materialsNode) {
            matout = findNodesWithAttribule(materialsNode, id, name);
        }
        if (matout == nullptr) {
            return;
        }
        const TiXmlElement* instance_effect = (const TiXmlElement*)matout->FirstChild();
        if (instance_effect) {
            materURL = safeAttribute(instance_effect, "url");
            materURL = materURL.substr(1, materURL.length()-1); //remove '#'
        }
        
        //texture has more names
        std::vector<std::string> diffuseTextureKeys;
        
        auto addChild = [&diffuseTextureKeys](const std::string child)->void{
            
            bool exist = false;
            
            for (int i=0; i<diffuseTextureKeys.size(); i++) {
                const std::string sub = diffuseTextureKeys[i];
                if (sub == child) {
                    exist = true;
                    break;
                }
            }
            
            if (!exist) {
                diffuseTextureKeys.push_back(child);
            }
        };
        
        const TiXmlNode* effectsNode = findNode(root, "library_effects");
        if (materURL.length() > 0 && effectsNode) {
            
            const TiXmlNode *effect = findNodesWithAttribule(effectsNode, id, materURL);
            if (effect) {
                const TiXmlElement *profile_COMMON = effect->FirstChildElement("profile_COMMON");
                if (profile_COMMON) {
                    
                    for (const TiXmlNode* sub_node = profile_COMMON->FirstChildElement(); sub_node; sub_node = sub_node->NextSibling()) {
                        
                        if (sub_node->ValueStr() == "newparam") {
                            
                            const char *sid = ((const TiXmlElement *)sub_node)->Attribute("sid");
                            if (sid) {
                                addChild(sid);
                            }
                            
                            const TiXmlElement *first = sub_node->FirstChildElement();
                            if (first) {
                                const TiXmlElement *second = first->FirstChildElement();
                                if (second) {
                                    const std::string s = second->GetText();
                                    if (s.length() > 0) {
                                        addChild(s);
                                    }
                                }
                            }
                            
                            continue;
                        }
                        
                        if (sub_node->ValueStr() == "technique") {
                            
                            const TiXmlElement *technique = (const TiXmlElement *)sub_node;
                            const TiXmlElement *phong = technique->FirstChildElement("phong");
                            
                            if (phong == nullptr) {
                                phong = technique->FirstChildElement("lambert");
                            }
                            
                            if (phong == nullptr) {
                                phong = technique->FirstChildElement("blinn");
                            }
                            
                            if (phong) {
                                std::string name = phong->ValueStr();
                                const TiXmlElement *diffuse = phong->FirstChildElement("diffuse");
                                if (diffuse) {
                                    const TiXmlElement *texture = diffuse->FirstChildElement("texture");
                                    if (texture) {
                                        const std::string diffText = safeAttribute(texture, "texture");
                                        if (diffText.size() > 0) {
                                            addChild(diffText);
                                        }
                                    }
                                }
                                
                            }
                            
                            continue;
                        }
                    }
                    
                }
            }
        }

        const TiXmlNode* imagesNode = findNode(root, "library_images");
        
        for (int i = 0; i < diffuseTextureKeys.size(); i++) {
            const std::string sub = diffuseTextureKeys[i];
            
            const TiXmlNode *img = findNodesWithAttribule(imagesNode, "id", sub);
            if (img) {
                const TiXmlElement *init_from = img->FirstChildElement("init_from");
                if (init_from) {
                    std::string imgName = init_from->GetText();
                    if (imgName.length()) {
                        mate.map_filepaths[trimesh::Material::DIFFUSE] = imgName;
                        mate.diffuse = trimesh::vec3(0.9);
                        mate.name = name;
                        break;
                    }
                }
            }
            
        }
    }

    void parseLibraryGeometries(const TiXmlNode* geoRoot, std::vector<trimesh::TriMesh*>& out, std::vector<std::string>& meshOfMaterialNames)
    {
        if (geoRoot == nullptr) {
            return;
        }
        
        std::vector<const TiXmlNode*> geometries;
        findNodes(geoRoot, geometries, "geometry");

        float spareTime = 1.0f;
        float perTime = geometries.size() > 0 ? spareTime / geometries.size() : 1.0f;
        float curTime = 0.0f;
        for (const TiXmlNode* geometry : geometries)
        {
            //curTime > 0 ? curTime += perTime : curTime;

    //            if (tracer && tracer->interrupt())
    //                return false;
            
            std::vector<const TiXmlNode*> meshes;
            findNodes(geometry, meshes, "mesh");

            int size = meshes.size();
            int curSize = 0;

            float perTime1 = meshes.size() > 0 ? perTime / meshes.size() : perTime;
            float curTime1 = curTime;
            for (const TiXmlNode* mesh : meshes)
            {
                int positioncount = 0;
                int normalcount = 0;
                //const char* position = nullptr;
                //const char* normal = nullptr;
                std::map<std::string, sourceGroup> sourceGroupMap;

                std::vector<const TiXmlNode*> sources;
                findNodes(mesh, sources, "source");
                
                for (const TiXmlNode* source : sources)
                {
                    const TiXmlElement* floatarryid = (const TiXmlElement*)source;
                    std::string strid = safeAttribute(floatarryid, "id");

                    const TiXmlNode* _floatarry = findNode(source, "float_array");
                    if (_floatarry == nullptr) {
                        continue;
                    }

                    const TiXmlElement* floatarry = (const TiXmlElement*)_floatarry;
                    sourceGroup _sourceGroup;
                    floatarry->Attribute("count", &_sourceGroup.count);
                    
                    const TiXmlNode* technique_common = findNode(source, "technique_common");
                    if (technique_common) {
                        const TiXmlElement* accessor = (const TiXmlElement*)findNode(technique_common, "accessor");
                        if (accessor) {
                            accessor->Attribute("stride", &_sourceGroup.stride);
                        }
                    }
                    
                    const char* floatarrydata = floatarry->GetText();
                    if (floatarrydata != nullptr)
                    {
                        _sourceGroup.position = floatarrydata;
                        sourceGroupMap.insert(std::pair<std::string, sourceGroup>(strid, _sourceGroup));
                    }
                    
                }
    //                if (tracer)
    //                {
    //                    curTime1 = curTime1 + perTime1 / 3.0;
    //                    curTime1 = curTime1 > 1.0 ? 1.0f : curTime1;
    //                    tracer->progress(curTime1);
    //                }

                std::vector<const TiXmlNode*> vertices;
                findNodes(mesh, vertices, "vertices");
                for (const TiXmlNode* vertice : vertices)
                {
                    const TiXmlElement* floatarry = (const TiXmlElement*)vertice;
                    std::string strid = safeAttribute(floatarry, "id");

                    std::vector<const TiXmlNode*> inputs;
                    findNodes(vertice, inputs, "input");
                    for (const TiXmlNode* node : inputs)
                    {
                        const TiXmlElement* sub_ele = (const TiXmlElement*)node;
                        std::string source = safeAttribute(sub_ele, "source");
                        if (source.length() > 1)
                        {
                            if (source.at(0) == '#')
                            {
                                source = source.substr(1, source.length()-1);
                                auto iter = sourceGroupMap.find(source);
                                if (iter != sourceGroupMap.end())
                                {
                                    sourceGroupMap.insert(std::pair<std::string, sourceGroup>(strid, iter->second));
                                }
                            }
                        }
                    }

                }

                std::vector<const TiXmlNode*> polylists;
                findNodes(mesh, polylists, "polylist");
                
                if (polylists.size()) {
                    parsePolylists(polylists, sourceGroupMap, out, meshOfMaterialNames);
                }
    //                if (tracer)
    //                {
    //                    curTime1 = curTime1 + perTime1 / 3.0;
    //                    curTime1 = curTime1 > 1.0 ? 1.0f : curTime1;
    //                    tracer->progress(curTime1);
    //                }

                std::vector<const TiXmlNode*> triangles;
                findNodes(mesh, triangles, "triangles");
                if (triangles.size()) {
                    parseTriangles(triangles, sourceGroupMap, out, meshOfMaterialNames);
                }
                
                std::vector<const TiXmlNode*> polygons;
                findNodes(mesh, polygons, "polygons");
                if (polygons.size()) {
                    parsePolygons(polygons, sourceGroupMap, out, meshOfMaterialNames);
                }
                
    //                if (tracer)
    //                {
    //                    curTime1 = curTime1 + perTime1 / 3.0;
    //                    curTime1 = curTime1 > 1.0 ? 1.0f : curTime1;
    //                    tracer->progress(curTime1);
    //                }
    //                curTime = curTime1;
                
                
            }
        }
    }

    void parseVisualGeometries(const TiXmlNode* vsRoot, std::vector<Instancegeometry>& out)
    {
        if (vsRoot == nullptr) {
            return;
        }
        
        for (const TiXmlNode* sub_node = vsRoot->FirstChild(); sub_node; sub_node = sub_node->NextSibling())
        {
            if (sub_node->Type() == TiXmlNode::TINYXML_ELEMENT)
            {
                const TiXmlElement* sub_element = (const TiXmlElement*)sub_node;
                
                const TiXmlNode* node = findNode(sub_element, "instance_geometry");
                if (node) {
                    //
                    Instancegeometry inst;
                    inst.id = safeAttribute(sub_element, "id");
                    std::string url = safeAttribute((TiXmlElement*)node, "url");
                    if (url.length()) {
                        inst.url = url.substr(1, url.length()-1);
                    }
                    
                    TiXmlElement* bind_material = (TiXmlElement*)findNode(node, "bind_material");
                    if (bind_material) {
                        TiXmlElement* technique_common = (TiXmlElement*)findNode(bind_material, "technique_common");
                        if (technique_common) {
                            TiXmlElement* instance_material = (TiXmlElement*)findNode(technique_common, "instance_material");
                            if (instance_material) {
                                inst.symbol = safeAttribute(instance_material, "symbol");
                                std::string target = safeAttribute(instance_material, "target");
                                if (target.length()) {
                                    inst.target = target.substr(1, target.length()-1);
                                }
                            }
                        }
                    }
                    
                    out.push_back(inst);
                } else {

                    const TiXmlElement *instance_controller = sub_element->FirstChildElement("instance_controller");
                    if (instance_controller) {
                        const TiXmlElement *bind_material = instance_controller->FirstChildElement("bind_material");
                        if (bind_material) {
                            const TiXmlElement *technique_common = bind_material->FirstChildElement("technique_common");
                            if (technique_common) {
                                const TiXmlElement *instance_material = technique_common->FirstChildElement("instance_material");
                                if (instance_material) {
                                    
                                    std::string symbol = safeAttribute(instance_material, "symbol");
                                    std::string target = safeAttribute(instance_material, "target");
                                    if (target.length()) {
                                        target = target.substr(1, target.length()-1);
                                    }
                                    
                                    Instancegeometry inst;
                                    inst.symbol = symbol;
                                    inst.target = target;
                                    out.push_back(inst);
                                }
                            }
                        }
                    }
                    
                }
                
            }
        }
    }

    bool DaeLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
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
        
        const TiXmlNode *visualScenes = findNode(root, "library_visual_scenes");
        const TiXmlNode *visual_scene = findNode(visualScenes, "visual_scene");
        
        std::vector<Instancegeometry> instanceGeo;
        parseVisualGeometries(visual_scene, instanceGeo);
        
        const TiXmlNode* _geometries = findNode(root, "library_geometries");
        
        std::vector<std::string> meshOfMaterialNames;
        
        if (_geometries) {
            parseLibraryGeometries(_geometries, out, meshOfMaterialNames);
        }
        
        for (int i = 0; i<meshOfMaterialNames.size(); i++) {
            std::string& sub = meshOfMaterialNames[i];

            if (sub.length()) {
                trimesh::Material m;
                parseMaterial(root, sub, m);
                
                if (m.diffuse.size() == 0 || m.name.size() == 0) {
                    if (instanceGeo.size() > i) {
                        parseMaterial(root, instanceGeo[i].target, m);
                    }
                }
                
                if (i < out.size()) {
                    trimesh::TriMesh*mesh = out[i];
                    //mesh->material = m;
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
