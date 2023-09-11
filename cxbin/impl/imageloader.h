#ifndef IMGPROC_IMAGELOADER_1608004787715_H
#define IMGPROC_IMAGELOADER_1608004787715_H

#include <string>
#include <vector>

namespace imgproc
{
	enum ImageFormat
	{
		IMG_FORMAT_PNG,
		IMG_FORMAT_JPG,
		IMG_FORMAT_BMP,
	};

	enum ImageDataFormat
	{
		FORMAT_RGB_565 = 3,
		FORMAT_RGBA_4444,
		FORMAT_RGBA_8888,
		FORMAT_GRAY_8,
	};

	class ImageData
	{
	public:
		unsigned char* data;
		int width;
		int height;
		ImageDataFormat format= ImageDataFormat::FORMAT_GRAY_8;
		struct point {
			int x;
			int y;
		};
	public:
		ImageData();
		ImageData(const ImageData& data);
		~ImageData();
		
		void release();
		void allocate(int w, int h);
		void resize(int w, int h);
		void gradient(unsigned char start, unsigned char end);
		bool valid();
		void fromFlipY(const ImageData& src);
		void clone( int w, int h, unsigned char* data);
		inline void blend(int x, int y, unsigned char value, unsigned char alpha)
		{
			if (alpha == 0 || x < 0 || x >= width || y < 0 || y >= height)
				return;

			unsigned char* dest = data + y * width + x;
			if (alpha == 255)
			{
				*dest = value;
				return;
			}

			unsigned char d = *dest;
			if (d == 255)
				return;

			unsigned char v = (unsigned char)((float)alpha * (float)value / 255.0f);
			if (255 - d < v)
				* dest = 255;
			else
				*dest = d + v;
		}
	};

	class GrayImage
	{
	public:
		GrayImage(){}
		~GrayImage() {}

		ImageData raw;
		ImageData alpha;
	};

	//type = 3 R5G6B5 , type = 4 R4G4B4A4 , type = 5 R8G8B8A8
	void fillImageData(ImageData& raw, ImageData* alpha, int width, int height, unsigned char* data, int type = 5, bool flipY = true);

	void freeImage_Initialise();
	void freeImage_Deinitialise();
	void loadImage(ImageData& data, const std::string& fileName);
	void loadImageNoDecode(const std::string& fileName, unsigned char*& target, unsigned& targetSize);
	void loadImage_freeImage(ImageData& data, const std::string& fileName);
	void loadImage_freeImage(ImageData& data, const std::string& extension, int fd);
	void loadImageFromMem_freeImage(unsigned char* source, unsigned sourceSize, ImageData& data, bool bgrFlag = false);
	void writeImage_freeImage(unsigned char* data, int width, int height, const std::string& fileName, ImageDataFormat format = ImageDataFormat::FORMAT_RGBA_8888);  //rgba
	void writeImage2Mem_freeImage(ImageData& data, unsigned char*& target, unsigned& targetSize, ImageFormat targetFormat);
	void getImageSizeFromMem_freeImage(unsigned char* source, unsigned sourceSize, unsigned& width, unsigned& height);
	ImageData* constructNewFreeImage(std::vector<ImageData*> data, ImageDataFormat format, std::vector<std::pair<ImageData::point, ImageData::point>>& offset);
	ImageData* scaleFreeImage(ImageData* imagedata, float scaleX, float scaleY);

	// encode the width and the height of texture to an Integer
	int encodeWH(int width, int height, int posOffset = 32768);
	void decodeWH(int& width, int& height, int code, int posOffset = 32768);
}

#endif // IMGPROC_IMAGELOADER_1608004787715_H
