#ifndef IMGPROC_OPTIMIZEENCHASER_1623637204192_H
#define IMGPROC_OPTIMIZEENCHASER_1623637204192_H

namespace imgproc
{
	struct EnchaseParam
	{
		float D;
		int blur;
		bool texcoordFlipY;
		EnchaseParam()
			: D(0.02f)
			, blur(0)
			, texcoordFlipY(true)
		{

		}
	};

	class Texture
	{
	public:
		Texture(int width, int height, unsigned char* texture);
		~Texture();

		bool texcoordGet(float s, float t, unsigned char& v) const;
		bool texcoordGet(int s, int t, unsigned char& v) const;

		int width() { return m_width; }
		int height() { return m_height; }

	protected:
		int m_width;
		int m_height;
		unsigned char* m_pixel;
		bool m_valid;
	};

	void enchasePosition(int vertexNum, float* position, float* UV,
		int uvStride, int width, int height, unsigned char* texture, const EnchaseParam& param);
}

#endif // IMGPROC_OPTIMIZEENCHASER_1623637204192_H