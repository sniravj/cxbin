#include "plugin3mf.h"

#include "lib3mf_implicit.hpp"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "ccglobal/tracer.h"
#include "ccglobal/log.h"
#include "imageproc/imageloader.h"
#include <map>

namespace cxbin
{
	_3mfLoader::_3mfLoader()
	{

	}

	_3mfLoader::~_3mfLoader()
	{

	}

	std::string _3mfLoader::expectExtension()
	{
		return "3mf";
	}

	bool _3mfLoader::tryLoad(FILE* f, size_t fileSize)
	{
		if (fileSize == 0)
			return false;

		LOGM("_3mfLoader try load file size [%d]", (int)fileSize);
		unsigned char* buffer = new unsigned char[fileSize + 1];
		Lib3MF::CInputVector<Lib3MF_uint8> Ibuffer(buffer, fileSize + 1);
		size_t readSize = fread(buffer, 1, fileSize, f);
		buffer[fileSize] = '\0';

		if(readSize < fileSize)
		{
			LOGM("_3mfLoader try read file size [%d]", (int)readSize);
			return false;
		}

		Lib3MF::PWrapper wrapper = Lib3MF::CWrapper::loadLibrary();
		Lib3MF::PModel model = wrapper->CreateModel();
		try
		{
			Lib3MF::PReader reader = model->QueryReader("3mf");
			reader->ReadFromBuffer(Ibuffer);
		}
		catch (const Lib3MF::ELib3MFException& exception)
		{
			delete[]buffer;
			return false;
		}

		Lib3MF::PMeshObjectIterator pmeshdata = model->GetMeshObjects();
		bool have3Mf = pmeshdata->Count() > 0;
		delete []buffer;

		return have3Mf;
	}

	bool _3mfLoader::load(FILE* f, size_t fileSize, std::vector<trimesh::TriMesh*>& out, ccglobal::Tracer* tracer)
	{
		if (fileSize == 0)
		{
			if (tracer)
			{
				tracer->failed("File is empty");
			}
			return false;
		}

		unsigned char* buffer = new unsigned char[fileSize];
		Lib3MF::CInputVector<Lib3MF_uint8> Ibuffer(buffer, fileSize);
		fread(buffer, 1, fileSize, f);

		Lib3MF::PWrapper wrapper = Lib3MF::CWrapper::loadLibrary();
		Lib3MF::PModel model = wrapper->CreateModel();
		Lib3MF::PReader reader = model->QueryReader("3mf");
		reader->ReadFromBuffer(Ibuffer);

		//����Uv����
		std::map<int, int> resourceID_uvIndex;
		std::map<int, int> resourceID_textrueIndex;
		std::vector<trimesh::vec2> tmp_UVs;
		Lib3MF::PTexture2DGroupIterator texture2DGroupIt = model->GetTexture2DGroups();
		trimesh::TriMesh* amesh = new trimesh::TriMesh();//out[0];
		std::vector<imgproc::ImageData*> imagedataV;
		int textrueIndex = 0;
		while (texture2DGroupIt->MoveNext())
		{
			Lib3MF::PTexture2DGroup aTexture2DGroup = texture2DGroupIt->GetCurrentTexture2DGroup();
			resourceID_uvIndex.insert(std::make_pair((int)aTexture2DGroup->GetUniqueResourceID(), tmp_UVs.size()));
			resourceID_textrueIndex.insert(std::make_pair((int)aTexture2DGroup->GetUniqueResourceID(), textrueIndex++));

			int uvIcount = aTexture2DGroup->GetCount();
			std::vector<Lib3MF_uint32> IDsBuffer;
			aTexture2DGroup->GetAllPropertyIDs(IDsBuffer);
			for (std::vector<Lib3MF_uint32>::iterator it = IDsBuffer.begin(); it != IDsBuffer.end(); it++)
			{
				Lib3MF::sTex2Coord aCoord = aTexture2DGroup->GetTex2Coord(*it);
				tmp_UVs.push_back(trimesh::vec2(aCoord.m_U, aCoord.m_V));
			}
			
			Lib3MF::PTexture2D aTexture2D = aTexture2DGroup->GetTexture2D();
			Lib3MF::PAttachment aAttachment = aTexture2D->GetAttachment();

			trimesh::Material amaterial;
			amesh->materials.push_back(amaterial);

			std::vector<Lib3MF_uint8> BufferBuffer;
			aAttachment->WriteToBuffer(BufferBuffer);
			imgproc::ImageData* newimagedatetemp = new imgproc::ImageData;
			newimagedatetemp->format = imgproc::ImageDataFormat::FORMAT_RGBA_8888;
			imgproc::loadImageFromMem_freeImage(BufferBuffer.data(), BufferBuffer.size(), *newimagedatetemp);
			imagedataV.push_back(newimagedatetemp);
		}
		std::swap(amesh->UVs, tmp_UVs);

		//����ͼƬ
		int widthMax = 0;
		int heightMax = 0;
		int widthOffset = 0;
		int heightOffset = 0;
		imgproc::ImageData* imageData = nullptr;
		int bytesPerPixel = 4;//FORMAT_RGBA_8888
		if (imagedataV.size() > 0)
		{
			std::vector<std::pair<imgproc::ImageData::point, imgproc::ImageData::point>> imageOffset;
			imageData = imgproc::constructNewFreeImage(imagedataV, imgproc::ImageDataFormat::FORMAT_RGBA_8888, imageOffset);
			if (imageData != nullptr)
			{
				widthMax = imageData->width / bytesPerPixel;
				heightMax = imageData->height;
				for (int i = 0; i < amesh->materials.size(); i++)
				{
					if (imagedataV[i] != nullptr)
					{
						trimesh::Material& material = amesh->materials[i];
						imgproc::ImageData::point startpos = imageOffset[i].first;
						imgproc::ImageData::point endpos = imageOffset[i].second;
						material.map_startUVs[trimesh::Material::DIFFUSE] = trimesh::vec2((float)startpos.x / widthMax, (float)startpos.y / heightMax);
						material.map_endUVs[trimesh::Material::DIFFUSE] = trimesh::vec2((float)endpos.x / widthMax, (float)endpos.y / heightMax);
					}
				}

				if (std::max(widthMax, heightMax) > 4096)
				{
					float scalevalue = (float)4096.0 / std::max(widthMax, heightMax);
					imageData = imgproc::scaleFreeImage(imageData, scalevalue, scalevalue);
				}
#ifdef TRIMESH_MAPBUF_RGB
                    amesh->map_bufferSize[trimesh::Material::DIFFUSE] = imgproc::encodeWH(imageData->width, imageData->height);
                    amesh->map_buffers[trimesh::Material::DIFFUSE] = imageData->data;
                    imageData->data = nullptr;
#else
				unsigned char* buffer;
				unsigned bufferSize;
				imgproc::writeImage2Mem_freeImage(*imageData, buffer, bufferSize, imgproc::ImageFormat::IMG_FORMAT_PNG);

				amesh->map_bufferSize[trimesh::Material::DIFFUSE] = bufferSize;
				amesh->map_buffers[trimesh::Material::DIFFUSE] = buffer;
#endif
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

		auto t2xf = [](const Lib3MF::sTransform& t) -> trimesh::fxform {
			trimesh::fxform xf = trimesh::fxform::identity();

			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 3; ++j)
					xf(i, j) = t.m_Fields[i][j];
			trimesh::transpose(xf);
			return xf;
		};

		auto processMesh = [&amesh, &tracer, &resourceID_uvIndex, &resourceID_textrueIndex](Lib3MF::PMeshObject mesh, const trimesh::fxform& xf) {
			if (!mesh)
				return;

			int faceCount = mesh->GetTriangleCount();

			amesh->faces.reserve(faceCount);
			amesh->vertices.reserve(3 * faceCount);

			int nfacets = faceCount;
			int calltime = nfacets / 10;
			if (calltime <= 0)
				calltime = nfacets;
			for (int nIdx = 0; nIdx < faceCount; nIdx++)
			{
				if (tracer && nIdx % calltime == 1)
				{
					tracer->progress((float)nIdx / (float)nfacets);
				}
				if (tracer && tracer->interrupt())
					return;

				Lib3MF::sTriangle faceIndex = mesh->GetTriangle(nIdx);
				Lib3MF::sPosition point1 = mesh->GetVertex(faceIndex.m_Indices[0]);
				Lib3MF::sPosition point2 = mesh->GetVertex(faceIndex.m_Indices[1]);
				Lib3MF::sPosition point3 = mesh->GetVertex(faceIndex.m_Indices[2]);

				trimesh::vec3 v1 = xf * trimesh::point(point1.m_Coordinates[0], point1.m_Coordinates[1], point1.m_Coordinates[2]);
				trimesh::vec3 v2 = xf * trimesh::point(point2.m_Coordinates[0], point2.m_Coordinates[1], point2.m_Coordinates[2]);
				trimesh::vec3 v3 = xf * trimesh::point(point3.m_Coordinates[0], point3.m_Coordinates[1], point3.m_Coordinates[2]);

				int v = amesh->vertices.size();
				amesh->vertices.push_back(v1);
				amesh->vertices.push_back(v2);
				amesh->vertices.push_back(v3);
				amesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));

				//����faceUVs
				Lib3MF::sTriangleProperties aProperty;
				mesh->GetTriangleProperties(nIdx, aProperty);
				std::map<int, int>::iterator it = resourceID_uvIndex.find((int)aProperty.m_ResourceID);
				if (it != resourceID_uvIndex.end() && (aProperty.m_PropertyIDs[0] != aProperty.m_PropertyIDs[1])
					&& (aProperty.m_PropertyIDs[0] != aProperty.m_PropertyIDs[2])
					&& (aProperty.m_PropertyIDs[1] != aProperty.m_PropertyIDs[2]))
				{
					std::map<int, int>::iterator it2 = resourceID_textrueIndex.find((int)aProperty.m_ResourceID);
					if (it2 != resourceID_textrueIndex.end())
					{
						amesh->faceUVs.push_back(trimesh::TriMesh::Face(aProperty.m_PropertyIDs[0] + it->second - 1, aProperty.m_PropertyIDs[1] + it->second - 1, aProperty.m_PropertyIDs[2] + it->second - 1));
						amesh->textureIDs.push_back(it2->second);
					}
				}
			}
		};
#if 1

		Lib3MF::PBuildItemIterator items = model->GetBuildItems();
		while (items->MoveNext())
		{
			if (tracer && tracer->interrupt())
				return false;

			Lib3MF::PBuildItem item = items->GetCurrent();
			Lib3MF::sTransform t = item->GetObjectTransform();
			trimesh::fxform xf = t2xf(t);

			Lib3MF_uint32 id = item->GetObjectResourceID();

			Lib3MF::PComponentsObjectIterator components = model->GetComponentsObjects();
			Lib3MF::PMeshObjectIterator meshes = model->GetMeshObjects();

			while (components->MoveNext())
			{
				Lib3MF::PComponentsObject comp = components->GetCurrentComponentsObject();
				if (comp->GetResourceID() == id)
				{
					Lib3MF_uint32 count = comp->GetComponentCount();
					for (Lib3MF_uint32 i = 0; i < count; ++i)
					{
						Lib3MF::PComponent c = comp->GetComponent(i);
						//Lib3MF::sTransform t = c->GetTransform();
						Lib3MF::PMeshObject mesh = model->GetMeshObjectByID(c->GetObjectResourceID());

						processMesh(mesh, xf);
					}
				}
			}

			while (meshes->MoveNext())
			{
				Lib3MF::PMeshObject mesh = meshes->GetCurrentMeshObject();
				if (mesh->GetResourceID() == id)
				{
					processMesh(mesh, xf);
				}
			}
		}

#else
		Lib3MF::PMeshObjectIterator pmeshdata = model->GetMeshObjects();
		while (pmeshdata->MoveNext())
		{
			if (tracer && tracer->interrupt())
				return false;

			Lib3MF::PMeshObject obj = pmeshdata->GetCurrentMeshObject();
			int faceCount = obj->GetTriangleCount();
			
			//trimesh::TriMesh* mesh = new trimesh::TriMesh();
			//out.push_back(mesh);

			amesh->faces.reserve(faceCount);
			amesh->vertices.reserve(3 * faceCount);

			int nfacets = faceCount;
			int calltime = nfacets / 10;
			if (calltime <= 0)
				calltime = nfacets;
			for (int nIdx = 0; nIdx < faceCount; nIdx++)
			{
				if (tracer && nIdx % calltime == 1)
				{
					tracer->progress((float)nIdx / (float)nfacets);
				}
				if (tracer && tracer->interrupt())
					return false;

				Lib3MF::sTriangle faceIndex = obj->GetTriangle(nIdx);
				Lib3MF::sPosition point1 = obj->GetVertex(faceIndex.m_Indices[0]);
				Lib3MF::sPosition point2 = obj->GetVertex(faceIndex.m_Indices[1]);
				Lib3MF::sPosition point3 = obj->GetVertex(faceIndex.m_Indices[2]);

				int v = amesh->vertices.size();
				amesh->vertices.push_back(trimesh::point(point1.m_Coordinates[0], point1.m_Coordinates[1], point1.m_Coordinates[2]));
				amesh->vertices.push_back(trimesh::point(point2.m_Coordinates[0], point2.m_Coordinates[1], point2.m_Coordinates[2]));
				amesh->vertices.push_back(trimesh::point(point3.m_Coordinates[0], point3.m_Coordinates[1], point3.m_Coordinates[2]));
				amesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));

				//����faceUVs
				Lib3MF::sTriangleProperties aProperty;
				obj->GetTriangleProperties(nIdx, aProperty);
				std::map<int, int>::iterator it = resourceID_uvIndex.find((int)aProperty.m_ResourceID);
				if (it != resourceID_uvIndex.end() && (aProperty.m_PropertyIDs[0] != aProperty.m_PropertyIDs[1])
												   && (aProperty.m_PropertyIDs[0] != aProperty.m_PropertyIDs[2])
												   && (aProperty.m_PropertyIDs[1] != aProperty.m_PropertyIDs[2]))
				{
					std::map<int, int>::iterator it2 = resourceID_textrueIndex.find((int)aProperty.m_ResourceID);
					if (it2 !=resourceID_textrueIndex.end())
					{
						amesh->faceUVs.push_back(trimesh::TriMesh::Face(aProperty.m_PropertyIDs[0] + it->second - 1, aProperty.m_PropertyIDs[1] + it->second - 1, aProperty.m_PropertyIDs[2] + it->second - 1));
						amesh->textureIDs.push_back(it2->second);
					}
				}
			}	
		}
#endif

		out.push_back(amesh);
		delete[]buffer;

		if (tracer)
		{
			tracer->success();
		}
		return true;
	}
}