#include <stdlib.h>
#include "pluginobj.h"

#include "stringutil/filenameutil.h"

#if __ANDROID__
#define _LIBCPP_HAS_NO_OFF_T_FUNCTIONS
#include "trimesh2/TriMesh.h"
#include "boost/filesystem.hpp"   // 包含所有需要的 Boost.Filesystem 声明
#else
#include "trimesh2/TriMesh.h"
#include "boost/filesystem.hpp"   // 包含所有需要的 Boost.Filesystem 声明
#endif

#include "cxbin/convert.h"
#include "ccglobal/tracer.h"
#include "imageproc/imageloader.h"
#include "boost/nowide/cstdio.hpp"

namespace cxbin
{
    struct ObjIndexedFace
    {
        void set(const int& num) { v.resize(num); n.resize(num); t.resize(num); }
        std::vector<int> v;
        std::vector<int> t;
        std::vector<int> n;
        int tInd = -1;
        bool edge[3];// useless if the face is a polygon, no need to have variable length array
        trimesh::ivec4 clr;
    };
	ObjLoader::ObjLoader()
	{

	}

	ObjLoader::~ObjLoader()
	{

	}

	std::string ObjLoader::expectExtension()
	{
		return "obj";
	}

	bool ObjLoader::tryLoad(FILE* f, size_t fileSize)
	{
		bool isObj = false;
		int times = 0;
		while (1) {
			stringutil::skip_comments(f);
			if (feof(f))
				break;
			char buf[1024];
			GET_LINE();
			if (LINE_IS("v ") || LINE_IS("v\t")) {
				float x, y, z;
				if (sscanf(buf + 1, "%f %f %f", &x, &y, &z) != 3) {
					break;
				}

				++times;
				if (times >= 3)
				{
					isObj = true;
					break;
				}
			}
		}

		return isObj;
	}

    void componentsSeparatedByString(std::string &from, char separator, std::vector<std::string> &out)
    {
        size_t leng = from.length();
        if (leng == 0) {
            return;
        }
        size_t location = 0;
        while (location < leng) {
            size_t tmp = from.find(separator, location);
            
            if (tmp == std::string::npos) {
                std::string sep = from.substr(location, leng-location);
                if (sep.length()) {
                    out.push_back(sep);
                }
                break;
            } else {
                std::string sep = from.substr(location, tmp-location);
                if (sep.length()) {
                    out.push_back(sep);
                }
                location = tmp + 1;
            }
        }
    }

    std::string trimStr(std::string& s)
    {
        size_t n = s.find_last_not_of(" \r\n\t");
        if (n != std::string::npos) {
            s.erase(n + 1, s.size() - n);
        }
        n = s.find_first_not_of(" \r\n\t");
        if (n != std::string::npos) {
            s.erase(0, n);
        }
        return s;
    }

	bool ObjLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
    {
        //std::vector<std::shared_ptr<AssociateFileInfo>> outfileinfo;
        //associateFileList(f, tracer, modelPath, outfileinfo);

		unsigned count = 0;
		unsigned calltime = fileSize / 10;
		int num = 0;

         std::vector<trimesh::vec3> tmp_vertices;
         std::vector<trimesh::vec2> tmp_UVs;
         std::vector<trimesh::vec3> tmp_normals;
         auto remapVertexIndex = [tmp_vertices](int vi) { return (vi < 0) ? tmp_vertices.size() + vi : vi - 1; };
         auto remapNormalIndex = [tmp_normals](int vi) { return (vi < 0) ? tmp_normals.size() + vi : vi - 1; };
         auto remapTexCoordIndex=[tmp_UVs](int vi) { return (vi < 0) ? tmp_UVs.size() + vi : vi - 1; };

        std::vector<trimesh::Material> materials;
        std::vector<ObjIndexedFace> indexedFaces;

        int  currentMaterialIdx = 0;
        //trimesh::Material defaultMaterial;					
        //defaultMaterial.index = currentMaterialIdx;
        //materials.push_back(defaultMaterial);
		std::string mtlName;

		while (1) {
			if (tracer && tracer->interrupt())
				return false;

//			stringutil::skip_comments(f);
            
			if (feof(f))
				break;
            char buf[MAX_OBJ_READLINE_LEN] = { 0 };
//            GET_LINE();
            fgets(buf, MAX_OBJ_READLINE_LEN, f);

			std::string str = buf;
			count += str.length();
			if (tracer && count >= calltime)
			{
				count = 0;
				num++;
				tracer->progress((float)num / 10.0);
			}
			
			if (LINE_IS("v ") || LINE_IS("v\t")) {
				float x, y, z;
				if (sscanf(buf + 1, "%f %f %f", &x, &y, &z) != 3) {
					if (tracer)
						tracer->failed("sscanf failed");
					return false;
				}
                tmp_vertices.push_back(trimesh::point(x, y, z));
			}
			else if (LINE_IS("vn ") || LINE_IS("vn\t")) {
				float x, y, z;
				if (sscanf(buf + 2, "%f %f %f", &x, &y, &z) != 3) {
					if (tracer)
						tracer->failed("sscanf failed");
					return false;
				}
                tmp_normals.push_back(trimesh::vec(x, y, z));
			}
            else if (LINE_IS("vt") || LINE_IS("vt\t")) {
                float x, y;
                if (sscanf(buf + 2, "%f %f", &x, &y) != 2) {
                    return false;
                }
                tmp_UVs.push_back(trimesh::vec2(x, y));
            }
			else if (LINE_IS("f ") || LINE_IS("f\t")) {
				
                std::string strOfFace = str.substr(2, str.length()-2);
                const char* ptr = strOfFace.c_str();
                int vi = 0, ti = 0, ni = 0;
                ObjIndexedFace   ff;
                while (*ptr != 0)
                {
                    // skip white space
                    while (*ptr == ' ') ++ptr;

                    if (sscanf(ptr, "%d/%d/%d", &vi, &ti, &ni) == 3)
                    {
                        ff.v.emplace_back(remapVertexIndex(vi));
                        ff.t.emplace_back(remapTexCoordIndex(ti));
                        ff.n.emplace_back(remapNormalIndex(ni));
                        ff.tInd = currentMaterialIdx;

                    }
                    else if (sscanf(ptr, "%d/%d", &vi, &ti) == 2)
                    {
                        ff.v.emplace_back(remapVertexIndex(vi));
                        ff.t.emplace_back(remapTexCoordIndex(ti));
                        ff.tInd = currentMaterialIdx;

                    }
                    else if (sscanf(ptr, "%d//%d", &vi, &ni) == 2)
                    {
                        ff.v.emplace_back(remapVertexIndex(vi));
                        ff.n.emplace_back(remapNormalIndex(ni));
                        ff.tInd = -1;
                    }
                    else if (sscanf(ptr, "%d", &vi) == 1)
                    {
                        ff.v.emplace_back(remapVertexIndex(vi));
                        ff.tInd = -1;
                    }

                    // skip to white space or end of line
                    while (*ptr != ' ' && *ptr != 0) ++ptr;

                }
                indexedFaces.push_back(ff);


			} 
            else if (LINE_IS("g ") || LINE_IS("o ")) {
                //结束上一个部件
//                if (model->faces.size() > 0) {
//                    model = new trimesh::TriMesh();
//                    out.push_back(model);
//                }
//
//                //开始下一个部件
//                std::string name = str.substr(2, str.length()-2);
//                name = trimStr(name);
                
            } 
            else if (LINE_IS("mtllib ")) {
                mtlName = str.substr(7, str.length()-7);
                mtlName = trimStr(mtlName);
                //mtl文件名
#if CC_SYSTEM_EMCC
                 loadMtl(mtlName, materials);
#else
                size_t loc = modelPath.find_last_of("/")+1;
                if (loc != std::string::npos) {
                    
                    const std::string materialFileName = modelPath.substr(0, loc) + mtlName;
                    printf("[mtl]: %s\n", materialFileName.c_str());
                    loadMtl(materialFileName, materials);
                }
#endif
                
            } 
            else if (LINE_IS("usemtl ")) {
                //mtllib should been readed before 
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                
                if (name.empty()) {
                    continue;
                }

                // emergency check. If there are no materials, the material library failed to load or was not specified
                // but there are tools that save the material library with the same name of the file, but do not add the 
                // "mtllib" definition in the header. So, we can try to see if this is the case
                if (materials.size() == 0) {
                    std::string materialFileName(modelPath);
                    materialFileName.replace(materialFileName.end() - 4, materialFileName.end(), ".mtl");
                    loadMtl(materialFileName, materials);
                }

                std::string materialName= name;

                bool found = false;
                unsigned i = 0;
                while (!found && (i < materials.size()))
                {
                    std::string currentMaterialName = materials[i].name;
                    if (currentMaterialName == materialName)
                    {
                        currentMaterialIdx = i;
                        trimesh::Material& material = materials[currentMaterialIdx];
                        trimesh::vec3 diffuseColor = material.diffuse;
                        unsigned char r = (unsigned char)(diffuseColor[0] * 255.0);
                        unsigned char g = (unsigned char)(diffuseColor[1] * 255.0);
                        unsigned char b = (unsigned char)(diffuseColor[2] * 255.0);
                        //unsigned char alpha = (unsigned char)(material.Tr * 255.0);
                        //trimesh::vec4 currentColor = trimesh::vec4(r, g, b, alpha);
                        found = true;
                    }
                    ++i;
                }

                if (!found)
                {
                    currentMaterialIdx = 0;
                }
            }

        }

		// XXX - FIXME
		// Right now, handling of normals is fragile: we assume that
		// if we have the same number of normals as vertices,
		// the file just uses per-vertex normals.  Otherwise, we can't
		// handle it.
        

        //if (tmp_vertices.size()*3== tmp_UVs.size()
        //    && tmp_UVs.size() == tmp_normals.size())
        {
            trimesh::TriMesh* modelmesh = new trimesh::TriMesh();
            modelmesh->mtlName = mtlName;
            std::swap(modelmesh->vertices, tmp_vertices);
            std::swap(modelmesh->UVs, tmp_UVs);
            std::swap(modelmesh->normals, tmp_normals);
            for (int i = 0; i < indexedFaces.size(); i++)
            {
                ObjIndexedFace& ff = indexedFaces[i];
                if (ff.v.size() == 3)
                {
                   modelmesh->faces.emplace_back(trimesh::TriMesh::Face(ff.v[0], ff.v[1], ff.v[2]));
                   if (ff.t.size() == 3)
                    {
                        modelmesh->faceUVs.emplace_back(trimesh::TriMesh::Face(ff.t[0], ff.t[1], ff.t[2]));
                        modelmesh->textureIDs.emplace_back(ff.tInd);
                    }
                    else
                    {
                        modelmesh->faceUVs.emplace_back(trimesh::TriMesh::Face(-1.0, -1.0, -1.0));
                        modelmesh->textureIDs.emplace_back(-1);
                    }
                    //if (ff.n.size()==3)
                    //{
                    //    modelmesh->faceVns.emplace_back(trimesh::TriMesh::Face(ff.n[0], ff.n[1], ff.n[2]));
                    //}
                }
                else if (ff.v.size() == 4)
                {
					modelmesh->faces.emplace_back(trimesh::TriMesh::Face(ff.v[0], ff.v[1], ff.v[2]));
                    modelmesh->faces.emplace_back(trimesh::TriMesh::Face(ff.v[0], ff.v[2], ff.v[3]));
					if (ff.t.size() == 4)
					{
						modelmesh->faceUVs.emplace_back(trimesh::TriMesh::Face(ff.t[0], ff.t[1], ff.t[2]));
                        modelmesh->textureIDs.emplace_back(ff.tInd);
                        modelmesh->faceUVs.emplace_back(trimesh::TriMesh::Face(ff.t[0], ff.t[2], ff.t[3]));
						modelmesh->textureIDs.emplace_back(ff.tInd);
					}
					else
					{
						modelmesh->faceUVs.emplace_back(trimesh::TriMesh::Face(-1.0, -1.0, -1.0));
						modelmesh->textureIDs.emplace_back(-1);
					}
                }
            }
            std::swap(modelmesh->materials, materials);
            //if(materials.size()>0)
            //    modelmesh->material = materials[0];

            loadMap(modelmesh);

            out.push_back(modelmesh);
        }

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}


    bool ObjLoader::loadMtl(const std::string& fileName, std::vector<trimesh::Material>& out)
    {
        FILE* f = fopen(fileName.c_str(), "rb");
        if (!f)
            return false;
        
        trimesh::Material *matePtr = nullptr;
        bool isValid = false;
        while (1) {
            stringutil::skip_comments(f);
            if (feof(f))
                break;
            char buf[1024];
            GET_LINE();
            std::string str = buf;
            if (LINE_IS("newmtl ")) {
                
                trimesh::Material newmat;
                newmat.index = out.size();
                out.push_back(newmat);
                
                matePtr = &out[out.size()-1];
                std::string name = str.substr(7, str.length()-7);
                matePtr->name = trimStr(name);
                
                isValid = true;
            } else if (LINE_IS("Ka ")) {
                
                if (!isValid) continue;
                
                float x, y, z;
                if (sscanf(buf + 2, "%f %f %f", &x, &y, &z) != 3) {
                    fclose(f);
                    return false;
                }
                matePtr->ambient = trimesh::vec3(x, y, z);
                
            } else if (LINE_IS("Kd ")) {
                
                if (!isValid) continue;
                
                float x, y, z;
                if (sscanf(buf + 2, "%f %f %f", &x, &y, &z) != 3) {
                    fclose(f);
                    return false;
                }
                matePtr->diffuse = trimesh::vec3(x, y, z);
                
            } else if (LINE_IS("Ks ")) {
                
                if (!isValid) continue;
                
                float x, y, z;
                if (sscanf(buf + 2, "%f %f %f", &x, &y, &z) != 3) {
                    fclose(f);
                    return false;
                }
                matePtr->specular = trimesh::vec3(x, y, z);
                
            } else if (LINE_IS("Ke ")) {
                
                if (!isValid) continue;
                
                float x, y, z;
                if (sscanf(buf + 2, "%f %f %f", &x, &y, &z) != 3) {
                    fclose(f);
                    return false;
                }
                matePtr->emission = trimesh::vec3(x, y, z);
                
            } else if (LINE_IS("Ns ")) {
                
                if (!isValid) continue;
                
                float x;
                if (sscanf(buf + 2, "%f", &x) != 1) {
                    fclose(f);
                    return false;
                }
                matePtr->shiness = x;
                
            } else if (LINE_IS("map_Ka ")) {
                
                if (!isValid) continue;
                
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
                    size_t loc = fileName.find_last_of("/");
#if CC_SYSTEM_EMCC
                    matePtr->map_filepaths[trimesh::Material::AMBIENT] = strs[strs.size() - 1];
#else
                    if (loc != std::string::npos) {

                        const std::string full = fileName.substr(0, loc) + "/" + strs[strs.size() - 1];
                        printf("[map_Ka]: %s\n", full.c_str());
                        matePtr->map_filepaths[trimesh::Material::AMBIENT] = full;

                    }
#endif   

                }
                
            } else if (LINE_IS("map_Kd ")) {
                
                if (!isValid) continue;
                
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
#if CC_SYSTEM_EMCC
                    matePtr->map_filepaths[trimesh::Material::DIFFUSE] = strs[strs.size() - 1];
#else
                    size_t loc = fileName.find_last_of("/");
                    if (loc != std::string::npos) {

                        const std::string full = fileName.substr(0, loc) + "/" + strs[strs.size() - 1];
                        printf("[map_Kd]: %s\n", full.c_str());
                        matePtr->map_filepaths[trimesh::Material::DIFFUSE] = full;

                    }
#endif

                }
                
            } else if (LINE_IS("map_Ks ")) {
                
                if (!isValid) continue;
                
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
#if CC_SYSTEM_EMCC
                    matePtr->map_filepaths[trimesh::Material::SPECULAR] = strs[strs.size() - 1];
#else
                    size_t loc = fileName.find_last_of("/");
                    if (loc != std::string::npos) {

                        const std::string full = fileName.substr(0, loc) + "/" + strs[strs.size() - 1];
                        printf("[map_Ks]: %s\n", full.c_str());
                        matePtr->map_filepaths[trimesh::Material::SPECULAR] = full;

                    }
#endif
                }
                
            } else if (LINE_IS("map_bump ")) {
            
                if (!isValid) continue;
                
                std::string name = str.substr(9, str.length()-9);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
#if CC_SYSTEM_EMCC
                    matePtr->map_filepaths[trimesh::Material::NORMAL] = strs[strs.size() - 1];
#else
                    size_t loc = fileName.find_last_of("/");
                    if (loc != std::string::npos) {
                        const std::string full = fileName.substr(0, loc) + "/" + strs[strs.size() - 1];
                        printf("[map_bump]: %s\n", full.c_str());
                        matePtr->map_filepaths[trimesh::Material::NORMAL] = full;
                    }
#endif
                }
            }
        }
        
        fclose(f);    

        return true;
    }

    //材质去重
    void ObjLoader::removeRepeatMaterial(trimesh::TriMesh* mesh)
    {
        std::vector<trimesh::Material>& materials = mesh->materials;
        std::vector<trimesh::Material> out;
        std::map<int, int> textureIDMap;
        
        for (int i = 0; i < materials.size(); i++)
        {
            trimesh::Material& material = materials[i];
            const std::string& fileName = material.map_filepaths[trimesh::Material::MapType::DIFFUSE];
            if (!fileName.empty()) {
                
                bool exist = false;
                
                for (int j = 0; j < out.size(); j ++) {
                    trimesh::Material& mat = out[j];
                    const std::string& sub = mat.map_filepaths[trimesh::Material::MapType::DIFFUSE];
                    if (fileName == sub) {
                        exist = true;
                        textureIDMap[i] = j;
                        break;
                    }
                }
                
                if (!exist) {
                    out.push_back(material);
                    
                    size_t idx = out.size() - 1;
                    textureIDMap[i] = (int)idx;
                }
            }
        }
        
        
        //修改顶点所属材质的索引
        std::vector<int>& tIDs = mesh->textureIDs;
        for (int i = 0; i < tIDs.size(); i++) {
            int &id = tIDs.at(i);
            id = textureIDMap[id];
        }
        
        materials.clear();
        materials = out;
    }


    bool ObjLoader::loadMap(trimesh::TriMesh* mesh)
    {
        if (mesh == nullptr)
            return false;

        removeRepeatMaterial(mesh);
        
        std::vector<trimesh::Material>& materials = mesh->materials;
        for (int type = 0; type < trimesh::Material::TYPE_COUNT; type++)
        {
            imgproc::ImageData* imageData = nullptr;
            int widthMax = 0;
            int heightMax = 0;
            int widthOffset = 0;
            int heightOffset = 0;
            int bytesPerPixel = 4;//FORMAT_RGBA_8888
#if CC_SYSTEM_EMCC
            if(type!=2)
            {
                continue;
            }
#endif
            std::vector<imgproc::ImageData*> imagedataV(materials.size(), nullptr);
            
            int sizeOf4k = 0;
            
            for (int i = 0; i < materials.size(); i++)
            {
                trimesh::Material& material = materials[i];
                const std::string& fileName = material.map_filepaths[type];
                if (!fileName.empty())
                {
                    imgproc::ImageData* newimagedatetemp = new imgproc::ImageData;
                    newimagedatetemp->format = imgproc::ImageDataFormat::FORMAT_RGBA_8888;
                    imgproc::loadImage_freeImage(*newimagedatetemp, fileName);
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
                        heightOffset += newimagedatetemp->height;
                        
                        if (newimagedatetemp->height >= 4000) {
                            sizeOf4k ++;
                        }
                        
                    }
                }
            }

            if (sizeOf4k >= 6) {
                
                for (int i = 0; i < imagedataV.size(); i++)
                {
                    imgproc::ImageData *temp = imagedataV[i];
                    
                    if (temp == nullptr) {
                        continue;
                    }
                    
                    if (temp->valid()) {
                        int w = temp->width / 4, h = temp->height;
                        float scalevalue = (float)1024.0 / std::max(w, h);
                        
                        imgproc::ImageData* scaled = imgproc::scaleFreeImage(temp, scalevalue, scalevalue);
                        
                        delete temp;
                        imagedataV[i] = scaled;
                        
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
                        
                        imgproc::ImageData* scaled = imgproc::scaleFreeImage(imageData, scalevalue, scalevalue);
                        delete imageData;
                        imageData = scaled;
                    }

#ifdef TRIMESH_MAPBUF_RGB
                    mesh->map_bufferSize[type] = imgproc::encodeWH(imageData->width, imageData->height);
                    mesh->map_buffers[type] = imageData->data;
                    imageData->data = nullptr;
#else
                    unsigned char* buffer;
                    unsigned bufferSize;
                    imgproc::writeImage2Mem_freeImage(*imageData, buffer, bufferSize, imgproc::ImageFormat::IMG_FORMAT_PNG);

                    mesh->map_bufferSize[type] = bufferSize;
                    mesh->map_buffers[type] = buffer;
#endif // TRIMESH_MAPBUF_RGB
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

    void ObjLoader::associateFileList(FILE* f, ccglobal::Tracer* tracer, const std::string& filePath, std::vector<std::shared_ptr<AssociateFileInfo>>& out)
    {
         std::string fileDir = filePath;

         size_t loc = filePath.find_last_of("/");
         if (loc != std::string::npos)
         {
//             std::vector<std::string> filenames;
             fileDir = filePath.substr(0, loc);
//         get_filenames(fileDir, filenames);
        }
        
        while (1) {
            if (tracer && tracer->interrupt())
                return;
            if (feof(f))
                break;
            char buf[MAX_OBJ_READLINE_LEN] = { 0 };
            fgets(buf, MAX_OBJ_READLINE_LEN, f);
            std::string str = buf;
            if (LINE_IS("mtllib "))
            {
                auto mtlName = str.substr(7, str.length() - 7);
                mtlName = trimStr(mtlName);
                size_t loc = mtlName.find_last_of("/");
                if (loc == std::string::npos)
                {
                    mtlName = fileDir + "/" + mtlName;
                }
                //mtl文件名

                printf("mtl absoluteDir: %s\n", mtlName.c_str());
                boost::filesystem::path path(mtlName);

                std::shared_ptr<AssociateFileInfo>associateInfor(new AssociateFileInfo());
                associateInfor->path = mtlName;
                
                if (boost::filesystem::exists(path))
                {
                    associateInfor->code = CXBinLoaderCode::no_error;
                    out.push_back(associateInfor);
                    
                    checkMtlCompleteness(mtlName, tracer, out);
                } else {
                    associateInfor->code = CXBinLoaderCode::file_mtl_not_exist;
                    out.push_back(associateInfor);
                    
                }
                goto WHILE_END;
            }
        }
    WHILE_END:
        ;
    }

    int ObjLoader::checkMtlCompleteness(std::string mtlFileName, ccglobal::Tracer* tracer, std::vector<std::shared_ptr<AssociateFileInfo>>& outFileinfo)
    {
//        std::shared_ptr<AssociateFileInfo>associateInfor(new AssociateFileInfo());

        FILE* f = fopen(mtlFileName.c_str(), "rb");
        if (!f)
        {
            std::shared_ptr<AssociateFileInfo>associateInfor(new AssociateFileInfo());
            associateInfor->path = mtlFileName;
            associateInfor->code = CXBinLoaderCode::file_invalid;
            outFileinfo.emplace_back(associateInfor);
            return -1;
        }

        while (1) {
            if (tracer && tracer->interrupt())
            {
                outFileinfo.clear();
                return -1;
            }
            stringutil::skip_comments(f);
            if (feof(f))
                break;
            char buf[1024];
            GET_LINE();
            std::string str = buf;
            if (LINE_IS("map_Ka ")) {
                std::string mapDestName;
                std::string name = str.substr(7, str.length() - 7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size())
                {
                    mapDestName = strs[0];
                    size_t loc = mapDestName.find_last_of("/");
                    if (loc == std::string::npos)
                    {
                        loc = mtlFileName.find_last_of("/");
                        if (loc != std::string::npos)
                        {
                            mapDestName = mtlFileName.substr(0, loc) + "/" + mapDestName;
                        }
                    }
                    boost::filesystem::path path(mapDestName);
                    std::shared_ptr<AssociateFileInfo>associateInfortemp(new AssociateFileInfo());
                    associateInfortemp->path = mapDestName;
                    associateInfortemp->code = CXBinLoaderCode::no_error;
                    if (!boost::filesystem::exists(path))
                    {
                        associateInfortemp->code = CXBinLoaderCode::file_not_exist;
                    }
                    outFileinfo.push_back(associateInfortemp);


                }

            }
            else if (LINE_IS("map_Kd ")) {
                std::string mapDestName;
                std::string name = str.substr(7, str.length() - 7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size())
                {
                    mapDestName = strs[0];
                    size_t loc = mapDestName.find_last_of("/");
                    if (loc == std::string::npos)
                    {
                        loc = mtlFileName.find_last_of("/");
                        if (loc != std::string::npos)
                        {
                            mapDestName = mtlFileName.substr(0, loc) + "/" + mapDestName;
                        }
                    }
                    boost::filesystem::path path(mapDestName);
                    std::shared_ptr<AssociateFileInfo>associateInfortemp(new AssociateFileInfo());
                    associateInfortemp->path = mapDestName;
                    associateInfortemp->code = CXBinLoaderCode::no_error;
                    if (!boost::filesystem::exists(path))
                    {
                        associateInfortemp->code = CXBinLoaderCode::file_not_exist;
                    }
                    outFileinfo.push_back(associateInfortemp);


                }
            }
            else if (LINE_IS("map_Ks ")) {
                std::string mapDestName;
                std::string name = str.substr(7, str.length() - 7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size())
                {
                    mapDestName = strs[0];
                    size_t loc = mapDestName.find_last_of("/");
                    if (loc == std::string::npos)
                    {
                        loc = mtlFileName.find_last_of("/");
                        if (loc != std::string::npos)
                        {
                            mapDestName = mtlFileName.substr(0, loc) + "/" + mapDestName;
                        }
                    }
                    boost::filesystem::path path(mapDestName);
                    std::shared_ptr<AssociateFileInfo>associateInfortemp(new AssociateFileInfo());
                    associateInfortemp->path = mapDestName;
                    associateInfortemp->code = CXBinLoaderCode::no_error;
                    if (!boost::filesystem::exists(path))
                    {
                        associateInfortemp->code = CXBinLoaderCode::file_not_exist;
                    }
                    outFileinfo.push_back(associateInfortemp);


                }
            }
            else if (LINE_IS("map_bump ")) {
                std::string mapDestName;
                std::string name = str.substr(9, str.length() - 9);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size())
                {
                    mapDestName = strs[0];
                    size_t loc = mapDestName.find_last_of("/");
                    if (loc == std::string::npos)
                    {
                        loc = mtlFileName.find_last_of("/");
                        if (loc != std::string::npos)
                        {
                            mapDestName = mtlFileName.substr(0, loc) + "/" + mapDestName;
                        }
                    }
                    boost::filesystem::path path(mapDestName);
                    std::shared_ptr<AssociateFileInfo>associateInfortemp(new AssociateFileInfo());
                    associateInfortemp->path = mapDestName;
                    associateInfortemp->code = CXBinLoaderCode::no_error;
                    if (!boost::filesystem::exists(path))
                    {
                        associateInfortemp->code = CXBinLoaderCode::file_not_exist;
                    }
                    outFileinfo.push_back(associateInfortemp);


                }

            }
        }
        fclose(f);

//        associateInfor->path = mtlFileName;
//        associateInfor->code = CXBinLoaderCode::no_error;
//        outFileinfo.push_back(associateInfor);
        return 0;
    }

	ObjSaver::ObjSaver()
	{

	}

	ObjSaver::~ObjSaver()
	{

	}

	std::string ObjSaver::expectExtension()
	{
        return "obj";
	}

	bool ObjSaver::save(FILE* f, trimesh::TriMesh* out, ccglobal::Tracer* tracer, std::string fileName)
	{
        return out->write(fileName);
	}

}
