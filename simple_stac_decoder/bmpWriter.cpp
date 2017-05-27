#include <stdio.h>
#include <stdint.h> // types
#include <string.h> // memset

#pragma pack(1)
struct STAC_BITMAPFILEHEADER
{
	uint16_t    bfType;
	uint32_t   bfSize;
	uint16_t    bfReserved1;
	uint16_t    bfReserved2;
	uint32_t   bfOffBits;
};
struct STAC_BITMAPINFOHEADER
{
	uint32_t      biSize;
	int32_t       biWidth;
	int32_t       biHeight;
	uint16_t       biPlanes;
	uint16_t       biBitCount;
	uint32_t      biCompression;
	uint32_t      biSizeImage;
	int32_t       biXPelsPerMeter;
	int32_t       biYPelsPerMeter;
	uint32_t      biClrUsed;
	uint32_t      biClrImportant;
};

void outputBmp(FILE* fOut, const float* const pflDepths)
{
	const int width = 640;
	const int height = 480;
	const int bytesPerPixel = 3;

	STAC_BITMAPFILEHEADER bm;
	STAC_BITMAPINFOHEADER bmHeader;
	memset(&bm, 0, sizeof(bm));
	bm.bfType = 'MB';
	bm.bfSize = width * height * 3 + sizeof(bm) + sizeof(bmHeader);
	bm.bfOffBits = sizeof(bm) + sizeof(bmHeader);
	

	memset(&bmHeader, 0, sizeof(bmHeader));
	bmHeader.biBitCount = bytesPerPixel * 8;
	bmHeader.biHeight = height;
	bmHeader.biWidth = width;
	bmHeader.biPlanes = 1;
	bmHeader.biSize = sizeof(bmHeader);
	bmHeader.biSizeImage = width * height * 3;

	fwrite(&bm, sizeof(bm), 1, fOut);
	fwrite(&bmHeader, sizeof(bmHeader), 1, fOut);

	for (int row = 479; row >= 0; row--)
	{
		unsigned char bufRow[width * bytesPerPixel];
		for (int col = 0; col < width; col++)
		{
			// need to convert each depth to r, g, and b.
			float depth = pflDepths[row * width + col];

			// let's do this as:
			// 0.00 - 0.33: red increases
			// 0.33 - 0.67: red decreases, green increases
			// 0.67 - 1.00: green decreases, blue increases
			float segStart = 0;
			float segLen = 0.33f;
			float r = 0;
			float g = 0;
			float b = 0;

			const float segments[] = { 0, 1500, 3000, 10000 };
			int ixMySegment = 0;
			float mySegmentPercent = 0;
			for (int segment = 1; segment < 4; segment++)
			{
				const float segStart = segments[segment-1];
				const float segEnd = segments[segment];
				if (depth >= segStart && depth < segEnd)
				{
					const float segLength = segEnd - segStart;
					mySegmentPercent = (depth - segStart) / segLength;
					ixMySegment = segment;
					break;
				}
			}

			switch (ixMySegment)
			{
			case 1: // red increasing
				r = mySegmentPercent;
				break;
			case 2: // red decreasing, green increasing
				r = 1 - mySegmentPercent;
				g = mySegmentPercent;
				break;
			case 3: // green decreasing, blue increasing
				g = 1 - mySegmentPercent;
				b = mySegmentPercent;
				break;
			}
			unsigned char byteRed = (unsigned char)(r * 255.0f);
			unsigned char byteGreen = (unsigned char)(g * 255.0f);
			unsigned char byteBlue = (unsigned char)(b * 255.0f);
			bufRow[col * bytesPerPixel] = byteBlue;
			bufRow[col * bytesPerPixel + 1] = byteGreen;
			bufRow[col * bytesPerPixel + 2] = byteRed;

		}
		fwrite(bufRow, sizeof(bufRow), 1, fOut);
	}
}