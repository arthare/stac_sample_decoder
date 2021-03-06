// License: Do whatever with the simple_stac_decoder code.  It took an hour to write, I'm not too attached to it.
//			Follow the Apple OS license in the lzfse project though.


#define _CRT_SECURE_NO_WARNINGS // go away, MSVC.  This ain't no secure program.

#include "lzfse.h"
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <Windows.h>
#include "bmpWriter.h"
#include "plyWriter.h"

using namespace std;

enum OUTPUTMODE
{
	OUTPUTMODE_ENCOURAGEMENT = 0,
	OUTPUTMODE_BIN = 1,
	OUTPUTMODE_BMP = 2,
	OUTPUTMODE_PLY = 3,
};

int main(int cArgs, char** pArgs)
{
	if (cArgs < 2)
	{
		printf("Usage: you gotta supply a filename.  Ex: \n>>simple_stac_decoder myFile.stac [bmp|bin]\n");
		return 0;
	}

	OUTPUTMODE eOutputMode = OUTPUTMODE_ENCOURAGEMENT;
	if (cArgs >= 3)
	{
		// 2nd argument is whether we're outputting frames
		if (!strcmp(pArgs[2], "bmp"))
		{
			eOutputMode = OUTPUTMODE_BMP;
		}
		else if (!strcmp(pArgs[2], "bin"))
		{
			eOutputMode = OUTPUTMODE_BIN;
		}
		else if (!strcmp(pArgs[2], "ply"))
		{
			eOutputMode = OUTPUTMODE_PLY;
		}
	}
	FILE* pFile = fopen(pArgs[1], "rb");
	if (!pFile)
	{
		cerr << "Could not find " << pArgs[1] << endl;
		return 1;
	}

	unsigned char* lzfseScratchBuffer = new unsigned char[lzfse_decode_scratch_size()];
	unsigned char* lzfseRecompressScratchBuffer = new unsigned char[lzfse_encode_scratch_size()];

	// constant-sized buffers that we're going to need.
	const int cbDecompressed = 640 * 480 * 2;
	unsigned char* depthBuffer = new unsigned char[640 * 480 * 2];
	float* depthInMillimetersBuffer = new float[640 * 480];

	unsigned char* compressedBuffer = 0;

	int ret = 0;

	// we've opened our file!
	// let's have at it
	int cElementsRead = 0;
	int cDepthFrames = 0;
	while (!feof(pFile))
	{
		uint64_t type = 0;
		cElementsRead = fread(&type, sizeof(type), 1, pFile);
		if (cElementsRead <= 0)
		{
			// I guess that's fine, but kinda weird that feof didn't go off...
			break;
		}

		double timestamp = 0;
		cElementsRead = fread(&timestamp, sizeof(timestamp), 1, pFile);
		if (cElementsRead <= 0)
		{
			goto unexpected_end;
		}

		uint64_t cbRemainingInChunk = 0;
		cElementsRead = fread(&cbRemainingInChunk, sizeof(cbRemainingInChunk), 1, pFile);
		if (cElementsRead <= 0)
		{
			goto unexpected_end;
		}

		// yes I know ftell doesn't do 64-bit filesizes.  This code is meant as an example.  If you want to improve it (and know the portable version of MS's _ftelli64),
		// send a pull request!
		const uint64_t startOfThisChunk = ftell(pFile);
		const uint64_t startOfNextChunk = startOfThisChunk + cbRemainingInChunk;

		FILE* pFrameOut = 0;


		// so now we know the type of our buffer, its timestamp, and how much is remaining in it.
		switch (type)
		{
		case 0: // accelerometer data
			break;
		case 1: // LZFSE-compressed 640x480 depth data
		{
			string strFilename;
			if (eOutputMode != OUTPUTMODE_ENCOURAGEMENT)
			{
				stringstream ssFilename;
				ssFilename << "frame" << cDepthFrames;
				const char* pszOpenMode = "wb";
				switch (eOutputMode)
				{
				case OUTPUTMODE_BIN:
					ssFilename << ".bin";
					break;
				case OUTPUTMODE_BMP:
					ssFilename << ".bmp";
					break;
				case OUTPUTMODE_PLY:
					ssFilename << ".ply";
					pszOpenMode = "w";
					break;
				}
				strFilename = ssFilename.str();
				pFrameOut = fopen(strFilename.c_str(), pszOpenMode);
			}
			cDepthFrames++;
			// let's just trust this file is authored correctly and that this won't make us crash!
			if (compressedBuffer) {
				delete[] compressedBuffer;
				compressedBuffer = 0;
			}
			compressedBuffer = new unsigned char[cbRemainingInChunk];
			if (!compressedBuffer)
			{
				printf("Ran out of memory while allocating a buffer for depth data from the chunk at filepos %lld\n", startOfThisChunk);
				goto out_of_memory;
			}
			cElementsRead = fread(compressedBuffer, cbRemainingInChunk, 1, pFile);
			if (cElementsRead <= 0)
			{
				goto unexpected_end;
			}

			lzfse_decode_buffer(depthBuffer, cbDecompressed, compressedBuffer, cbRemainingInChunk, lzfseScratchBuffer);

			// because it seemed that the depthInMillimeters frame from the structure sensor had slightly more depth resolution than the 11-bit shiftData
			// buffer, I used it.  However, since my iPad mini 2 couldn't compress 640*480*float buffers at a good frame rate, I stretched each float out by
			// multiplying by 6.5536 so that a 0-10m range would strech to the range that a short can hold.  Don't like it?  make your own depth type.
			const int cPixels = 640 * 480;
			const unsigned short* psDepthBuffer = (const unsigned short*)depthBuffer;
			
			for (int x = 0; x < cPixels; x++)
			{
				if (psDepthBuffer[x] == 65535 || psDepthBuffer[x] == 0)
				{
					// out-of-range pixel
					depthInMillimetersBuffer[x] = 0;
				}
				else
				{
					depthInMillimetersBuffer[x] = (float)psDepthBuffer[x] / 6.5536f;
				}
			}

			if (pFrameOut)
			{
				// if we're outputting frames, then let's output them!
				switch (eOutputMode)
				{
				case OUTPUTMODE_BIN:
					fwrite(depthInMillimetersBuffer, 640 * 480 * sizeof(depthInMillimetersBuffer[0]), 1, pFrameOut);
					printf("Wrote 640x480 floats to %s\n", strFilename.c_str());
					break;
				case OUTPUTMODE_BMP:
					outputBmp(pFrameOut, depthInMillimetersBuffer);
					printf("Wrote bmp %s\n", strFilename.c_str());
					break;
				case OUTPUTMODE_PLY:
					// projection is handled inside WritePlyVertex.  I assume if I do another 3D output format that'll get split out
					WritePlyFile(pFrameOut, depthInMillimetersBuffer);
					printf("Wrote PLY frame to %s\n", strFilename.c_str());
					break;
				}
			}
			else
			{
				// you're not in file-write mode (aka the 2nd parameter was not included or was zero).  I'll give you some motivation I guess?
				printf("Decompressed depth at timestamp %.02f.  Do something cool with it!\n", timestamp);
				for (int row = 0; row < 480; row++)
				{
					for (int col = 0; col < 640; col++)
					{
						// fish out the depth pixel here
						float thisPixelDepth = depthInMillimetersBuffer[row * 640 + col];

						// it is now up to you, intrepid programmer, to make something of this data.
						// make a model!  make some measurements!  do whatever!
					}
				}

			}

		    break;
		}
		case 2: // a crappier way I was sending depth data originally
			break;
		case 3: // file metadata
			break;
		}

		if (pFrameOut)
		{
			fclose(pFrameOut);
		}

		// warning: may not deal well with filesizes > 2gb.
		fseek(pFile, startOfNextChunk, SEEK_SET);
		continue;
	out_of_memory:
	unexpected_end:
		printf("Unexpected end of file at byte %d\n", ftell(pFile));
	    ret = 1;
		break;

	}

	fclose(pFile);

	delete[] lzfseScratchBuffer;
	delete[] lzfseRecompressScratchBuffer;
	delete[] depthBuffer;
	delete[] depthInMillimetersBuffer;
	delete[] compressedBuffer;

	return ret;
}