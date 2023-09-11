#include "optimizeenchaser.h"
#include "trimesh2/Vec.h"

namespace imgproc
{
	Texture::Texture(int width, int height, unsigned char* texture)
		:m_width(width), m_height(height), m_pixel(texture)
	{
		m_valid = m_width > 0 && m_height > 0 && m_pixel;
	}

	Texture::~Texture()
	{

	}

	bool Texture::texcoordGet(int s, int t, unsigned char& v) const
	{
		float fs = (float)s / (float)m_width;
		float ts = (float)t / (float)m_height;
		return texcoordGet(fs, ts, v);
	}

	bool Texture::texcoordGet(float s, float t, unsigned char& v) const
	{
		if (s < 0 || s > 1 || t < 0 || t > 1 || !m_valid)
		{
			return false;
		}
		float tv = t * (m_height - 1);
		float sv = s * (m_width - 1);

		int y = round(tv);
		int x = round(sv);

		float epson = 0.001f;
		float xper = abs(x - sv);
		float yper = abs(y - tv);

		// ��Ҫ˫���Բ�ֵ
		if (yper >= epson)
		{
			unsigned char* data1 = m_pixel + y * m_width + x;
			unsigned char* data2 = ((tv - y) > 0) ? data1 + m_width : data1 - m_width;

			float v1 = (float)(*data1);
			float v2 = (float)(*data2);

			// ��һ�����Բ�ֵ
			if (xper >= 0.01)   // �� 0.01 ��Χ�ڣ��򲻽��в�ֵ
			{
				bool b = (sv - x) >= 0;

				float _t = b ? *(data1 + 1) : *(data1 - 1);
				v1 = v1 * (1 - xper) + _t * xper;

				_t = b ? *(data2 + 1) : *(data2 - 1);
				v2 = v2 * (1 - xper) + _t * xper;
			}

			v = (unsigned char)(v1 * (1 - yper) + v2 * yper);   // �ڶ������Բ�ֵ
		}
		else    // ֻ��Ҫһ�����Բ�ֵ
		{
			if (xper >= epson)   // �� 0.01 ��Χ�ڣ��򲻽��в�ֵ
			{
				unsigned char* data1 = m_pixel + y * m_width + x;
				float v1 = *data1;

				bool b = (sv - x) >= 0;
				float _t = b ? *(data1 + 1) : *(data1 - 1);
				v = (unsigned char)(v1 * (1 - xper) + _t * xper);
			}
			else
			{
				v = m_pixel[y * m_width + x];
			}
		}
		return true;
	}

	void enchasePosition(int vertexNum, float* position, float* UV, int uvStride,
		int width, int height, unsigned char* texture, const EnchaseParam& param)
	{
		if(width <= 0 || height <= 0 || texture == nullptr)
			return;

		Texture tex(width, height, texture);
		for (int i = 0; i < vertexNum; ++i)
		{
			trimesh::vec3* p = (trimesh::vec3*)(position + 3 * i);
			trimesh::vec2* xy = (trimesh::vec2*)(UV + uvStride * i);

			unsigned char t = 0;
			float dy = param.texcoordFlipY ? (1.0f - xy->y) : xy->y;
			if (tex.texcoordGet(xy->x, dy, t))
			{
				trimesh::vec3 n = trimesh::normalized(*p);
				*p += n * ((float)(255 - t) / 255.0f) * param.D;
			}
		}
	}
}