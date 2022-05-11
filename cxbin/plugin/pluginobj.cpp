#include <stdlib.h>
#include "pluginobj.h"

#include "stringutil/filenameutil.h"
#include "trimesh2/TriMesh.h"
#include "cxbin/convert.h"
#include "ccglobal/tracer.h"

namespace cxbin
{
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
		trimesh::TriMesh* model = new trimesh::TriMesh();
		out.push_back(model);
		unsigned count = 0;
		unsigned calltime = fileSize / 10;
		int num = 0;

        std::vector<trimesh::vec3> tmp_normals;
        std::vector<trimesh::vec3> tmp_vertices;
        std::vector<trimesh::vec2> tmp_UVs;
        
        std::vector<trimesh::Material> mates;
        
		while (1) {
			if (tracer && tracer->interrupt())
				return false;

//			stringutil::skip_comments(f);
            
			if (feof(f))
				break;
            char buf[65536];
//            GET_LINE();
            fgets(buf, 65536, f);

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
                strOfFace = trimStr(strOfFace);
                //一个图元，可能包含超过3个顶点
                std::vector<std::string> face;
                componentsSeparatedByString(strOfFace, ' ', face);
                size_t pointSize = face.size();
                
                for (size_t i = 2; i < pointSize; i++) {
                    
                    std::string firstStrOfPoint = face[0];
                    //一个顶点可能包含多个不同的属性
                    std::vector<std::string> firstAttributeOut;
                    componentsSeparatedByString(firstStrOfPoint, '/', firstAttributeOut);
                    
                    switch (firstAttributeOut.size()) {
                            
                        case 1:
                        {
                            //顶点
                            int idx0 = atoi(firstStrOfPoint.c_str()) - 1;
                            int idx1 = atoi(face[i-1].c_str()) - 1;
                            int idx2 = atoi(face[i].c_str()) - 1;
                            
                            model->faces.push_back(trimesh::TriMesh::Face(idx0, idx1, idx2));
                        }
                            break;
                            
                        case 2:
                        {
                            
                            if (tmp_UVs.size() > 0) {
                                //顶点+纹理
                                
                                //第一个顶点
                                int idx0 = atoi(firstAttributeOut[0].c_str()) - 1;
                                int uvidx0 = atoi(firstAttributeOut[1].c_str()) - 1;
                                
                                //第二个顶点
                                std::string secondStrOfPoint = face[i-1];
                                std::vector<std::string> secondAttributeOut;
                                componentsSeparatedByString(secondStrOfPoint, '/', secondAttributeOut);
                                
                                int idx1 = atoi(secondAttributeOut[0].c_str()) - 1;
                                int uvidx1 = atoi(secondAttributeOut[1].c_str()) - 1;
                                
                                //第三个顶点
                                std::string thirdStrOfPoint = face[i];
                                std::vector<std::string> thirdAttributeOut;
                                componentsSeparatedByString(thirdStrOfPoint, '/', thirdAttributeOut);
                                
                                int idx2 = atoi(thirdAttributeOut[0].c_str()) - 1;
                                int uvidx2 = atoi(thirdAttributeOut[1].c_str()) - 1;
                                
                                model->faces.push_back(trimesh::TriMesh::Face(idx0, idx1, idx2));
                                model->faceUVs.push_back(trimesh::TriMesh::Face(uvidx0, uvidx1, uvidx2));
                                
                            } else if (tmp_normals.size() > 0)
                            {
                                //顶点+法线
                                
                                //第一个顶点
                                int idx0 = atoi(firstAttributeOut[0].c_str()) - 1;
                                
                                //第二个顶点
                                std::string secondStrOfPoint = face[i-1];
                                std::vector<std::string> secondAttributeOut;
                                componentsSeparatedByString(secondStrOfPoint, '/', secondAttributeOut);
                                
                                int idx1 = atoi(secondAttributeOut[0].c_str()) - 1;
                            
                                //第三个顶点
                                std::string thirdStrOfPoint = face[i];
                                std::vector<std::string> thirdAttributeOut;
                                componentsSeparatedByString(thirdStrOfPoint, '/', thirdAttributeOut);
                                
                                int idx2 = atoi(thirdAttributeOut[0].c_str()) - 1;
                                model->faces.push_back(trimesh::TriMesh::Face(idx0, idx1, idx2));
                                
                            }
                        }
                            break;
                        case 3:
                        {
                            //顶点+纹理+法线
                            
                            //第一个顶点
                            int idx0 = atoi(firstAttributeOut[0].c_str()) - 1;
                            int uvidx0 = atoi(firstAttributeOut[1].c_str()) - 1;
                            
                            
                            //第二个顶点
                            std::string secondStrOfPoint = face[i-1];
                            std::vector<std::string> secondAttributeOut;
                            componentsSeparatedByString(secondStrOfPoint, '/', secondAttributeOut);
                            
                            int idx1 = atoi(secondAttributeOut[0].c_str()) - 1;
                            int uvidx1 = atoi(secondAttributeOut[1].c_str()) - 1;

                            //第三个顶点
                            std::string thirdStrOfPoint = face[i];
                            std::vector<std::string> thirdAttributeOut;
                            componentsSeparatedByString(thirdStrOfPoint, '/', thirdAttributeOut);
                            
                            int idx2 = atoi(thirdAttributeOut[0].c_str()) - 1;
                            int uvidx2 = atoi(thirdAttributeOut[1].c_str()) - 1;
                            
                            model->faces.push_back(trimesh::TriMesh::Face(idx0, idx1, idx2));
                            model->faceUVs.push_back(trimesh::TriMesh::Face(uvidx0, uvidx1, uvidx2));
                            
                        }
                            break;
                            
                        default:
                            break;
                    }
                }
			} else if (LINE_IS("g ") || LINE_IS("o ")) {
                //结束上一个部件
//                if (model->faces.size() > 0) {
//                    model = new trimesh::TriMesh();
//                    out.push_back(model);
//                }
//
//                //开始下一个部件
//                std::string name = str.substr(2, str.length()-2);
//                name = trimStr(name);
                
            } else if (LINE_IS("mtllib ")) {
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                //mtl文件名
                
                size_t loc = modelPath.find_last_of("/");
                if (loc != std::string::npos) {
                    
                    const std::string full = modelPath.substr(0, loc) + "/" + name;
                    printf("[mtl]: %s\n", full.c_str());
                    
                    loadMtl(full, mates);
                }
                
                
            } else if (LINE_IS("usemtl ")) {
                continue;
                //"usemtl"指定了材质之后，以后的面都是使用这一材质，直到遇到下一个"usemtl"来指定新的材质。
                
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                
                if (name.empty()) {
                    continue;
                }
                
                //结束上一个部件
                if (model->faces.size() > 0) {
                    trimesh::TriMesh *sameNameModel = nullptr;
                    
                    for (int i = 0; i < out.size(); i++) {
                        trimesh::TriMesh *sub = out[i];
                        if (sub->material.name == name) {
                            sameNameModel = sub;
                            break;
                        }
                    }
                    
                    if (sameNameModel) {
                        
                        model = sameNameModel;
                        
                    } else {
                        model = new trimesh::TriMesh();
                        out.push_back(model);
                    }
                    
                }
                
                if (mates.size()) {
                    for (trimesh::Material mat : mates) {
                        if (mat.name == name) {
                            model->material = mat;
                            break;
                        }
                    }
                } else {
                    model->material.name = name;
                }
                
            }
        }

		// XXX - FIXME
		// Right now, handling of normals is fragile: we assume that
		// if we have the same number of normals as vertices,
		// the file just uses per-vertex normals.  Otherwise, we can't
		// handle it.
        
        for (int i=0; i<out.size(); i++) {
            trimesh::TriMesh *mesh = out[i];
            mesh->vertices = tmp_vertices;
            mesh->UVs = tmp_UVs;
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
                    matePtr->ambientMap = strs[strs.size()-1];
                }
                
            } else if (LINE_IS("map_Kd ")) {
                
                if (!isValid) continue;
                
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
                    matePtr->diffuseMap = strs[strs.size()-1];
                }
                
            } else if (LINE_IS("map_Ks ")) {
                
                if (!isValid) continue;
                
                std::string name = str.substr(7, str.length()-7);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
                    matePtr->specularMap = strs[strs.size()-1];
                }
                
            } else if (LINE_IS("map_bump ")) {
            
                if (!isValid) continue;
                
                std::string name = str.substr(9, str.length()-9);
                name = trimStr(name);
                std::vector<std::string> strs;
                componentsSeparatedByString(name, ' ', strs);
                if (strs.size()) {
                    std::vector<std::string>::iterator it = strs.end()-1;
                    matePtr->normalMap = *it;
                }
                
            }
        }
        
        fclose(f);
            
        return true;
    }
}
