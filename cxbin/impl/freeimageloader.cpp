#include "imageloader.h"

#if HAVE_FREEIMAGE
#include "FreeImage.h"
#endif

namespace imgproc
{
#if HAVE_FREEIMAGE
/** Generic image loader
	@param lpszPathName Pointer to the full file name
	@param flag Optional load flag constant
	@return Returns the loaded dib if successful, returns NULL otherwise
*/
FIBITMAP* GenericLoader(const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName, 0);
	if (fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
	}
	// check that the plugin has reading capabilities ...
	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// ok, let's load the file
		FIBITMAP* dib = FreeImage_Load(fif, lpszPathName, flag);
		// unless a bad file format, we are done !
		return dib;
	}
	return NULL;
}

FIBITMAP* GenericLoader(FREE_IMAGE_FORMAT fif, int flag, int fd) 
{
	// check that the plugin has reading capabilities ...
	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// ok, let's load the file
		FIBITMAP* dib = FreeImage_LoadFD(fif, fd, flag);
		// unless a bad file format, we are done !
		return dib;
	}
	return NULL;
}

/** Generic image writer
	@param dib Pointer to the dib to be saved
	@param lpszPathName Pointer to the full file name
	@param flag Optional save flag constant
	@return Returns true if successful, returns false otherwise
*/
bool GenericWriter(FIBITMAP* dib, const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	BOOL bSuccess = FALSE;

	if (dib) {
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
		if (fif != FIF_UNKNOWN) {
			// check that the plugin has sufficient writing and export capabilities ...
			WORD bpp = FreeImage_GetBPP(dib);
			if (FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp)) {
				// ok, we can save the file
				bSuccess = FreeImage_Save(fif, dib, lpszPathName, flag);
				// unless an abnormal bug, we are done !
			}
		}
	}
	return (bSuccess == TRUE) ? true : false;
}

// ----------------------------------------------------------

/**
	FreeImage error handler
	@param fif Format / Plugin responsible for the error
	@param message Error message
*/
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
	printf("\n*** ");
	if (fif != FIF_UNKNOWN) {
		printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
	}
	printf("%s ", message);
	printf(" ***\n");
}

#endif 

#if HAVE_FREEIMAGE
	void convert2Gray(ImageData& data, FIBITMAP* dib)
	{
		const int nBitCounts = 8;
		int nBpp = FreeImage_GetBPP(dib);
		BYTE* imgData = FreeImage_GetBits(dib);
		int width = FreeImage_GetWidth(dib);
		int height = FreeImage_GetHeight(dib);

		int rowSize = (((width * nBpp) + 31) >> 5) << 2;
		if (imgData && width > 0 && height > 0)
		{
			data.allocate(width, height);

			BYTE nIntensity;
			int j, k;
			switch (nBpp)
			{
			case 32: //32Î»Í¼ï¿½ï¿½ï¿½ï¿½alphaÍ¨ï¿½ï¿½ï¿½ï¿½×ªï¿½ï¿½Îª8Î»ï¿½Ò¶ï¿½Í¼
				for (j = 0; j < height; j++)
				{
					for (k = 0; k < width; k++)
					{
						nIntensity = (BYTE)(0.33333 * imgData[j * rowSize + 4 * k] +
							0.33333 * imgData[j * rowSize +4 * k + 1] + 0.33333 * imgData[j * rowSize + 4 * k + 2]);
						
						*(data.data + j * width + k) = nIntensity;
					}
				}
				break;
			case 24: //24Î»Í¼ï¿½ï¿½×ªÎª8Î»ï¿½Ò¶ï¿½Í¼
				for (j = 0; j < height; j++)
				{
					for (k = 0; k < width; k++)
					{
						nIntensity = (BYTE)(0.33333 * imgData[j * rowSize + 3 * k] +
							0.33333 * imgData[j * rowSize + 3 * k + 1] + 0.33333 * imgData[j * rowSize + 3 * k + 2]);

						*(data.data + j * width + k) = nIntensity;
					}
				}
				break;
			case 8: //8Î»Î±ï¿½ï¿½É«×ªÎª8Î»ï¿½Ò¶ï¿½Í¼
				for (j = 0; j < height; j++)
				{
					for (k = 0; k < width; k++)
					{
						FreeImage_GetPixelIndex(dib, k, j, &nIntensity);
						RGBQUAD* ptrRGB = FreeImage_GetPalette(dib);
						nIntensity = (BYTE)(0.299 * ptrRGB[nIntensity].rgbRed +
							0.587 * ptrRGB[nIntensity].rgbGreen + 0.114 * ptrRGB[nIntensity].rgbBlue);

						*(data.data + j * width + k) = nIntensity;
					}
				}
			}
		}
	}
	bool convertBaseFormat(ImageData& data, FIBITMAP* dib, bool invertFlag = true)
	{
		bool ret = true;
		switch (data.format)
		{
		case ImageDataFormat::FORMAT_RGBA_8888:
		{
			//! »ñÈ¡Êý¾ÝÖ¸Õë
			FIBITMAP* newdib = FreeImage_ConvertTo32Bits(dib);

			BYTE* pixels = (BYTE*)FreeImage_GetBits(newdib);
			int     width = FreeImage_GetWidth(newdib);
			int     height = FreeImage_GetHeight(newdib);
			data.allocate(width * 4, height);
			data.format = ImageDataFormat::FORMAT_RGBA_8888;
			for (int i = 0; i < width * height * 4; i += 4)
			{
				if (invertFlag)
				{
					data.data[i] = pixels[i + 2];//bgr->rgb
					data.data[i + 1] = pixels[i + 1];
					data.data[i + 2] = pixels[i];//bgr->rgb
				}
				else
				{
					data.data[i] = pixels[i];
					data.data[i + 1] = pixels[i + 1];
					data.data[i + 2] = pixels[i + 2];
				}

				data.data[i + 3] = pixels[i + 3] > 0 ? pixels[i + 3] : 255;
			}
			FreeImage_Unload(newdib);
		}
		break;
		default:
		{
			convert2Gray(data, dib);
		}
		}
		return ret;

	}

#endif
	void freeImage_Deinitialise()
	{
#if HAVE_FREEIMAGE
        FreeImage_DeInitialise();
#endif
    }


	void freeImage_Initialise()
	{
#if HAVE_FREEIMAGE
		FreeImage_Initialise(FALSE);
#endif
	}
	void loadImage_freeImage(ImageData& data, const std::string& fileName)
	{
#if HAVE_FREEIMAGE
		FreeImage_SetOutputMessage(FreeImageErrorHandler);

		FIBITMAP* dib = GenericLoader(fileName.c_str(), 0);
		if (dib != NULL) {

			convertBaseFormat(data, dib);
			// free the dib
			FreeImage_Unload(dib);
		}else
			data.release();
#endif
	}

	void loadImage_freeImage(ImageData& data, const std::string& extension, int fd)
	{
#if HAVE_FREEIMAGE
		FreeImage_SetOutputMessage(FreeImageErrorHandler);

		FREE_IMAGE_FORMAT format = FIF_UNKNOWN;
		if (extension == "jpg" || extension == "jpeg")
			format = FIF_JPEG;
		else if (extension == "png")
			format = FIF_PNG;

		FIBITMAP* dib = GenericLoader(format, 0, fd);
		if (dib != NULL) {

			convertBaseFormat(data, dib);
			// free the dib
			FreeImage_Unload(dib);
		}else
			data.release();
#endif
	}

	void loadImageFromMem_freeImage(unsigned char* source, unsigned sourceSize, ImageData& data, bool bgrFlag)
	{
#if HAVE_FREEIMAGE
		FIMEMORY* fmem = FreeImage_OpenMemory(source, sourceSize);
		FREE_IMAGE_FORMAT fif = FREE_IMAGE_FORMAT::FIF_UNKNOWN;
		fif = FreeImage_GetFileTypeFromMemory(fmem, 0);
		FIBITMAP* dib = FreeImage_LoadFromMemory(fif, fmem, 0);

		if (dib != NULL) {

			convertBaseFormat(data, dib, bgrFlag);
			// free the dib
			FreeImage_Unload(dib);
		}
		else
			data.release();

		FreeImage_CloseMemory(fmem);
#endif
	}

	void writeImage_freeImage(unsigned char* data, int width, int height, const std::string& fileName, ImageDataFormat format)
	{
#if HAVE_FREEIMAGE
		FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

		// check the file signature and deduce its format
		// (the second argument is currently not used by FreeImage)
		fif = FreeImage_GetFileType(fileName.c_str(), 0);
		if (fif == FIF_UNKNOWN) {
			// no signature ?
			// try to guess the file format from the file extension
			fif = FreeImage_GetFIFFromFilename(fileName.c_str());
		}
		// check that the plugin has reading capabilities ...
		if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsWriting(fif)) 
		{
			FIBITMAP* dib = nullptr;
			switch (format)
			{
			case imgproc::FORMAT_GRAY_8:
				dib = FreeImage_ConvertFromRawBits(data, width, height, width, 8, 0, 0, 0);
				break;
			case imgproc::FORMAT_RGBA_4444:
				dib = FreeImage_ConvertFromRawBits(data, width / 2, height, width, 16, 0, 0, 0);
				break;
			case imgproc::FORMAT_RGBA_8888:
				dib = FreeImage_ConvertFromRawBits(data, width / 4, height, width, 32, 0, 0, 0);
				break;
			case imgproc::FORMAT_RGB_565:
				dib = FreeImage_ConvertFromRawBits(data, width / 2, height, width, 16, 0, 0, 0);
				break;
			default:
				break;
			}
			if (dib == nullptr)
				return;

			FreeImage_Save(fif, dib, fileName.c_str(), 0);
			FreeImage_Unload(dib);
		}
#endif
	}

	void writeImage2Mem_freeImage(ImageData& data, unsigned char*& target, unsigned& targetSize, ImageFormat targetFormat)
	{
#if HAVE_FREEIMAGE
		FREE_IMAGE_FORMAT fif = FIF_PNG;
		switch (targetFormat)
		{
		case IMG_FORMAT_PNG:
			fif = FIF_PNG;
			break;
		case IMG_FORMAT_JPG:
			fif = FIF_JPEG;
			break;
		case IMG_FORMAT_BMP:
			fif = FIF_BMP;
			break;
		default:
			fif = FIF_PNG;
			break;
		}

		FIMEMORY* fmem = FreeImage_OpenMemory();
		FIBITMAP* dib = nullptr;
		switch (data.format)
		{
		case imgproc::FORMAT_GRAY_8:
			dib = FreeImage_ConvertFromRawBits(data.data, data.width, data.height, data.width, 8, 0, 0, 0);
			break;
		case imgproc::FORMAT_RGBA_4444:
			dib = FreeImage_ConvertFromRawBits(data.data, data.width / 2, data.height, data.width, 16, 0, 0, 0);
			break;
		case imgproc::FORMAT_RGBA_8888:
			dib = FreeImage_ConvertFromRawBits(data.data, data.width / 4, data.height, data.width, 32, 0, 0, 0);
			break;
		case imgproc::FORMAT_RGB_565:
			dib = FreeImage_ConvertFromRawBits(data.data, data.width / 2, data.height, data.width, 16, 0, 0, 0);
			break;
		default:
			break;
		}

		if (dib == nullptr)
			return;

		FreeImage_SaveToMemory(fif, dib, fmem, 0);

		long memSize = FreeImage_TellMemory(fmem);
		targetSize = memSize;
		target = new unsigned char[memSize];

		FreeImage_SeekMemory(fmem, 0L, SEEK_SET);

		FreeImage_ReadMemory(target, sizeof(unsigned char), memSize, fmem);

		FreeImage_CloseMemory(fmem);
#endif
	}

	void getImageSizeFromMem_freeImage(unsigned char* source, unsigned sourceSize, unsigned& width, unsigned& height)
	{
#if HAVE_FREEIMAGE
		FIMEMORY* fmem = FreeImage_OpenMemory(source, sourceSize);
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(fmem, 0);
		FIBITMAP* dib = FreeImage_LoadFromMemory(fif, fmem, 0);

		if (dib != NULL)
		{
			width = FreeImage_GetWidth(dib);
			height = FreeImage_GetWidth(dib);
		}
		else
		{
			width = -1;
			height = -1;
		}

		FreeImage_CloseMemory(fmem);
#endif
	}

	ImageData* constructNewFreeImage(std::vector<ImageData*> imagedata, ImageDataFormat format, std::vector<std::pair<ImageData::point, ImageData::point>> &offset)
	{
		imgproc::ImageData* dataret = nullptr;
		#if HAVE_FREEIMAGE
		//int H = FreeImage_GetHeight(dib);

		//µÃµ½Í¼Ïñ¿í¶Èint W = FreeImage_GetWidth(dib);

		//µÃµ½Í¼ÏñÏñËØ BYTE* data = FreeImage_GetBits(dib);

		//µÃµ½Í¼ÏñÎ»Éî int bpp = FreeImage_GetBpp(dib);

		//µÃµ½x, yÏñËØ RGBQUAD color; FreeImage_GetPixelColor(dib, x, y, &color);

		//Ð´Èëx, yÏñËØ FreeImage_SetPixelColor(dib, x, y, color);

		//¿ª±ÙÐÂÍ¼Ïñ FIBITMAP* re = FreeImage_Allocate(W, H, bpp);

		//¿½±´exifÐÅÏ¢ FreeImage_CloneMetadata(dib, re);
		int widthMax = 0;
		int heightMax = 0;
		int widthTotal = 0;
		int heightTotal = 0;
		int widthoffset = 0;
		int heightoffset = 0;
		int bytesPerPixel = 4;//FORMAT_RGBA_8888
		int bpp = 32;
		switch (format)
		{
		case FORMAT_RGBA_8888:
			bytesPerPixel = 4;
			bpp = 32;
			break;
			//case FORMAT_RGB_565:
			//case FORMAT_RGBA_4444:
			//	bytesPerPixel = 2;
			//	break;
		default://other format is not test
		{
			printf("other format is not test");
			return nullptr;

		}
		}
		for (auto dataPtr: imagedata)
		{

			if (dataPtr == nullptr)
			{
				continue;//should be not empty
			}
			if (dataPtr->format != format)//channels must be same
			{
				return nullptr;
			}

			widthMax = widthMax > dataPtr->width?widthMax:dataPtr->width;
			heightMax = heightMax > dataPtr->height? heightMax :dataPtr->height;
			widthTotal += dataPtr->width/ bytesPerPixel;
			heightTotal += dataPtr->height;
		}
		widthMax = widthMax/ bytesPerPixel;
		heightMax = heightMax;
		FIBITMAP* dibptr =FreeImage_Allocate(widthMax, heightTotal, bpp);
		if (dibptr == nullptr)
		{
			return nullptr;
		}

		offset.resize(imagedata.size());
		int offsetIndex=0;
		for (int index=0;index<imagedata.size();index++)
		{
			auto &dataPtr = imagedata[index];
			if (dataPtr == nullptr)
			{
				widthoffset = 0;
				ImageData::point startpos = { widthoffset,heightoffset };
				ImageData::point endpos = { widthoffset ,heightoffset };
				offset[offsetIndex++] = std::make_pair(startpos, endpos);
				continue;
			}
			for (int indexW = 0; indexW < dataPtr->width/ bytesPerPixel; indexW++)
			{
				for (int indexH = 0; indexH < dataPtr->height; indexH++)
				{
					RGBQUAD rgb;
					rgb.rgbRed =   dataPtr->data[dataPtr->width * indexH + indexW * bytesPerPixel];
					rgb.rgbGreen = dataPtr->data[dataPtr->width * indexH + indexW * bytesPerPixel +1];
					rgb.rgbBlue =  dataPtr->data[dataPtr->width * indexH + indexW * bytesPerPixel +2];
					FreeImage_SetPixelColor(dibptr, widthoffset + indexW, heightoffset + indexH, &rgb);
				}

			}
			ImageData::point startpos = { widthoffset ,heightoffset };
			heightoffset += dataPtr->height;
			widthoffset = dataPtr->width / bytesPerPixel;
			ImageData::point endpos = { widthoffset ,heightoffset };
			offset[offsetIndex++] = std::make_pair(startpos, endpos);
			widthoffset = 0;
				
		}
		//FreeImage_Save(FREE_IMAGE_FORMAT::FIF_BMP, dibptr, "test.bmp", 0);
			if(dibptr)
			{
				dataret = new imgproc::ImageData;
				dataret->format = format;
				convertBaseFormat(*dataret, dibptr);
				FreeImage_Unload(dibptr);

			}
			#endif
			return dataret;
	}
	ImageData* scaleFreeImage(ImageData* imagedata, float scaleX, float scaleY)
	{
		imgproc::ImageData* dataret = nullptr;
		#ifdef HAVE_FREEIMAGE
		int bytesPerPixel = 4;//FORMAT_RGBA_8888
		int bpp = 32;
		switch (imagedata->format)
		{
		case FORMAT_RGBA_8888:
			bytesPerPixel = 4;
			bpp = 32;
			break;
			//case FORMAT_RGB_565:
			//case FORMAT_RGBA_4444:
			//	bytesPerPixel = 2;
			//	break;
		default://other format is not test
		{
			printf("other format is not test");
			return nullptr;

		}
		}

		FIBITMAP* dibOrignal = FreeImage_Allocate(imagedata->width / bytesPerPixel, imagedata->height, bpp);
		for (int indexW = 0; indexW < imagedata->width / bytesPerPixel; indexW++)
		{
			for (int indexH = 0; indexH < imagedata->height; indexH++)
			{
				RGBQUAD rgb;
				rgb.rgbRed = imagedata->data[imagedata->width * indexH + indexW * bytesPerPixel];
				rgb.rgbGreen = imagedata->data[imagedata->width * indexH + indexW * bytesPerPixel + 1];
				rgb.rgbBlue = imagedata->data[imagedata->width * indexH + indexW * bytesPerPixel + 2];
				FreeImage_SetPixelColor(dibOrignal, indexW, indexH, &rgb);
			}

		}
		int newW = imagedata->width * scaleX / bytesPerPixel;
		int newH = imagedata->height * scaleY;
		FIBITMAP* newdib = FreeImage_Rescale(dibOrignal, newW, newH);
		FreeImage_Unload(dibOrignal);
		dataret = new imgproc::ImageData;
		dataret->format = imagedata->format;
		convertBaseFormat(*dataret, newdib);
		FreeImage_Unload(newdib);
		#endif
		return dataret;
	}

	int encodeWH(int width, int height, int posOffset)
	{
		return width * posOffset + height;
	}

	void decodeWH(int& width, int& height, int code, int posOffset)
	{
		width = code / posOffset;
		height = code % posOffset;
	}
}
