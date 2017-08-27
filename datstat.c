#include <stdio.h>
#include <stdlib.h>


typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned long int dword;


struct LevelInfo
{
	word number;
	word time;
	word chips;
} levelInfo;


void displayLevelStats(const byte* pLevelData, word levelSize)
	{
	const byte* p = pLevelData;
	int i;
	#define bump(T) *(((const T*)p)++)

	levelInfo = bump(struct LevelInfo);
	p += sizeof(word);
	for (i = 0; i < 2; ++i)
		{
		word layerSize = bump(word);
		p += layerSize;
		}
	p += sizeof(word) + sizeof(byte) + sizeof(byte);
	printf("%3d. %s\n", levelInfo.number, p);

	#undef bump
	}


int processFile(const char* szDatFileName)
	{
	FILE* fp = NULL;
	word nLevels = 0, l;

	fp = fopen(szDatFileName, "rb");
	if (!fp)
		{
		printf("Unable to open file: %s\n", szDatFileName);
		return 1;
		}

	fseek(fp, sizeof(dword), SEEK_SET);
	fread(&nLevels, sizeof(word), 1, fp);

	for (l = 1; l <= nLevels; ++l)
		{
		word levelSize = 0;
		byte* pLevelData = NULL;

		fread(&levelSize, sizeof(word), 1, fp);
		if (levelSize == 0) break;

		pLevelData = malloc(levelSize);
		fread(pLevelData, levelSize, 1, fp);
		displayLevelStats(pLevelData, levelSize);
		free(pLevelData);
		}

	fclose(fp);
	return 0;
	}


int main(int nParams, const char* szParam[])
	{
	if (nParams != 1+1)
		{
		puts("Syntax:   datstat  datfile [> outfile]");
		return -1;
		}

	return processFile(szParam[1]);
	}
