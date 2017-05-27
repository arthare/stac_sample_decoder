#include <ostream>
#include <sstream>
#include <iostream>

using namespace std;

void WritePlyHeader(FILE* pFile, int cVertices)
{
	fprintf(pFile, "ply\n");
	fprintf(pFile, "format ascii 1.0\n");
	fprintf(pFile, "comment VCGLIB generated\n");
	fprintf(pFile, "element vertex %d\n", cVertices);
	fprintf(pFile, "property float x\n");
	fprintf(pFile, "property float y\n");
	fprintf(pFile, "property float z\n");
	fprintf(pFile, "end_header\n");
}

void WritePlyFile(FILE* pFile, float* depthInMillimetersBuffer)
{
	const int width = 640;
	const int height = 480;

	const double kinectFovWidth = 58.5;
	const double kinectFovHeight = 46.6;
	const double PI = 3.14159;

	stringstream ssHeader;
	stringstream ssVertices;

	int ix = 0;
	int cValidVertices = 0;
	for (int row = 0; row < 480; row++)
	{
		for (int col = 0; col < 640; col++)
		{
			const double sampleMillimeters = depthInMillimetersBuffer[ix];
			if (sampleMillimeters > 500 && sampleMillimeters < 8000)
			{
				cValidVertices++;
			}
			ix++;
		}
	}

	WritePlyHeader(pFile, cValidVertices);


	ix = 0;
	for (int row = 0; row < 480; row++)
	{
		const double rowpct = (double)row / (double)480;
		const double rowAngleDegrees = (rowpct - 0.5) * kinectFovHeight;
		const double rowAngleRad = rowAngleDegrees * PI / 180.0;

		float y = sin(rowAngleRad);

		for (int col = 0; col < 640; col++)
		{
			const double sampleMillimeters = depthInMillimetersBuffer[ix];
			if (sampleMillimeters > 500 && sampleMillimeters < 8000)
			{
				const double colpct = (double)col / (double)640;
				const double colAngleDegrees = (colpct - 0.5) * kinectFovWidth;
				const double colAngleRad = colAngleDegrees * PI / 180.0;
				const double sampleMeters = (double)sampleMillimeters / 1000.0;
				// depth represents distance perpindicular to scanner
				const double x = sin(colAngleRad);
				const double z = sampleMeters;

				float finalX = x * z;
				float finalY = y * z;
				float finalZ = z;
				fprintf(pFile, "%f %f %f\n", finalX, finalY, finalZ);
			}
			ix++;
		}

		fflush(pFile);
	}

}