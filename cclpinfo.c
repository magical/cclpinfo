/* cclpinfo v1.3 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PASSWORD_MASK 0x99
#define CHIP_TILE (word) 0x02

// This is used when seperating the filename from the path.
// When compiling for *nix, it should be changed to '/'
#define DIR_SEPERATOR '\\'

typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned long int dword;

struct
{
	word number;
	word time;
	word chips;
	word totalchips;
	char *title;
	char *password;
	char *hint;
	word upperlayersize;
	byte *upperlayer;
	word lowerlayersize;
	byte *lowerlayer;
} levelInfo;

struct
{
	const char *datfile_name;
	const char *savefile_name;
	bool display_passwords:1;
	bool display_time:1;
	bool display_hints:1;
	bool display_chips:1;
} options;

word count_tiles(byte *layerdata, word layersize, byte search_tile)
	{
	word count = 0;
	int i = 0;

	while (i < layersize)
		{
		if (layerdata[i] == search_tile)
			{
			count += 1;
			i += 1;
			}
		else if (layerdata[i] == 0xff)
			{
			if (layerdata[i+2] == search_tile)
				{
				count += layerdata[i+1];
				}
			i += 3;
			}
		else
			{
			i += 1;
			}
		}	
	return count;	
	}

void displayLevelStats(FILE *fp, word levelsize, FILE *out)
	{
	int l;
	char *j;
	word length;

	l = levelsize;
	
	levelInfo.title = NULL;
	levelInfo.password = NULL;
	levelInfo.hint = NULL;

	fread(&levelInfo.number, sizeof(word), 1, fp); // the level number
	fread(&levelInfo.time, sizeof(word), 1, fp); // the time limit
	fread(&levelInfo.chips, sizeof(word), 1, fp); // # chips required
	fseek(fp, sizeof(word), SEEK_CUR); // 0x0001

	fread(&levelInfo.upperlayersize, sizeof(word), 1, fp); // length of upper layer data
	if (options.display_chips)
		{
		levelInfo.upperlayer = malloc(levelInfo.upperlayersize);
		fread(levelInfo.upperlayer, sizeof(byte), levelInfo.upperlayersize, fp);
		}
	else
		{
		fseek(fp, levelInfo.upperlayersize, SEEK_CUR);
		}
	l -= levelInfo.upperlayersize;
	
	fread(&levelInfo.lowerlayersize, sizeof(word), 1, fp); // length of lower layer data
	if (options.display_chips)
		{
		levelInfo.lowerlayer = malloc(levelInfo.lowerlayersize);
		fread(levelInfo.lowerlayer, sizeof(byte), levelInfo.lowerlayersize, fp);
		}
	else
		{
		fseek(fp, levelInfo.lowerlayersize, SEEK_CUR);
		}
	l -= levelInfo.lowerlayersize;

	fread(&length, sizeof(word), 1, fp); // length of misc data
	l -= sizeof(word) * 7;

	if (options.display_chips)
		{
		levelInfo.totalchips = 0;
		levelInfo.totalchips += count_tiles(levelInfo.upperlayer, levelInfo.upperlayersize, CHIP_TILE);
		levelInfo.totalchips += count_tiles(levelInfo.lowerlayer, levelInfo.lowerlayersize, CHIP_TILE);
		}
	
	while (l > 0)
		{
		byte fieldnum, fieldlength;
		fread(&fieldnum, sizeof(byte), 1, fp);
		fread(&fieldlength, sizeof(byte), 1, fp);
		switch (fieldnum)
			{
			// 1: time limit
			// 2: chips
			case 3: // level title
				levelInfo.title = malloc(fieldlength);
				fread(levelInfo.title, fieldlength, 1, fp);
				break;
			// 4: trap connections
			// 5: clone connections
			case 6: // password
				levelInfo.password = malloc(fieldlength);
				fread(levelInfo.password, fieldlength, 1, fp);
				j = levelInfo.password;
				while (*j)
					{
					*j = *j ^ PASSWORD_MASK;
					j++;
					}
				//printf("%s", levelInfo.password);
				break;
			case 7: // level hint
				levelInfo.hint = malloc(fieldlength);
				fread(levelInfo.hint, fieldlength, 1, fp);
				break;
			// I don't know what 8 and 9 are for
			// 10: creature list
			default:
				fseek(fp, fieldlength, SEEK_CUR);
			}
		l -= fieldlength + 2;
		}
	
	fprintf(out, "%3d. %-35s ", levelInfo.number, levelInfo.title);
	if (options.display_passwords)
		{
		fprintf(out, "\t%s", levelInfo.password);
		}
	if (options.display_time)
		{
		if (levelInfo.time == 0)
			{
			fputs("\t---", out);
			}
		else
			{
			fprintf(out, "\t%03d", levelInfo.time);
			}
		}
	if (options.display_chips)
		{
		if (levelInfo.chips == levelInfo.totalchips)
			{
			fprintf(out, "\t%d", levelInfo.chips);
			}
		else
			{
			fprintf(out, "\t%d/%d", levelInfo.chips, levelInfo.totalchips);
			}
		}

	fputc('\n', out);

	if (options.display_hints && levelInfo.hint != NULL)
		{
		fprintf(out, "     %s\n", levelInfo.hint);
		}
	#define nfree(p) if (p != NULL) free(p)
	nfree(levelInfo.title);
	nfree(levelInfo.password);
	nfree(levelInfo.hint);
	#undef nfree
	}

const char* extract_filename(const char *path)
	{
	const char *filename = path;

	while (*path)
		{
		if (*path == DIR_SEPERATOR)
			{
			filename = path+1;
			}
		path++;
		}
	return filename;
	}

int processFile(const char* szDatFileName)
	{
	FILE* fp = NULL;
	word nLevels = 0, l;
	dword signature;

	fp = fopen(szDatFileName, "rb");
	if (!fp)
		{
		printf("Unable to open file: %s\n", szDatFileName);
		return 1;
		}
	
	fread(&signature, sizeof(dword), 1, fp);
	if (signature != 0x0002aaac && signature != 0x0102aaac)
		{
		fprintf(stderr, "%s is not a valid Chip's Challenge file.\n", szDatFileName);
		fclose(fp);
		return 1;
		}
	fread(&nLevels, sizeof(word), 1, fp);
	
	printf("%s\n\n", extract_filename(szDatFileName));
	
	printf("  #. %-35s%s%s%s\n", "Title", (options.display_passwords?"\tPass":""), (options.display_time?"\tTime":""), (options.display_chips?"\tChips":""));
	for (l = 1; l <= nLevels; ++l)
		{
		word levelSize = 0;
		fread(&levelSize, sizeof(word), 1, fp);
		if (levelSize == 0) break;
		displayLevelStats(fp, levelSize, stdout);
		}

	fclose(fp);
	return 0;
	}

int main(int argc, const char *argv[])
	{
	int i, j;
	
	if (argc <= 1)
		{
		puts("cclpinfo v1.3 written by Andrew E. and Madhav Shanbhag");
		puts("");
		printf("Usage:\t%s file [-ptch]\n", argv[0]);
		puts("");
		puts("\tfile\tThe location of the .dat file to use");
		puts("\t-p\tdisplay passwords");
		puts("\t-t\tdisplay the time limits");
		puts("\t-c\tdisplay chips required/available");
		puts("\t-h\tdisplay the level hints");
		puts("");
		puts("For more information, read README.TXT");
		return -1;
		}
	
	// initialize the options
	options.datfile_name = NULL;
	options.savefile_name = NULL;
	options.display_passwords = false;
	options.display_time = false;
	options.display_chips = false;
	options.display_hints = false;

	//parse the command-line options
	for (i = 1; i < argc; i+=1)
		{
		if (argv[i][0] == '-' )
			{
			for (j = 1; j < strlen(argv[i]); j+=1)
				{
				switch (argv[i][j])
					{
					case 'p':
						options.display_passwords = true;
						break;
					case 't':
						options.display_time = true;
						break;
					case 'c':
						options.display_chips = true;
						break;
					case 'h':
						options.display_hints = true;
						break;
					default:
						fprintf(stderr, "Warning: option -%c is invalid\n", argv[i][j]);
					}
				}
			}
		else if (options.datfile_name == NULL)
			{
			options.datfile_name = argv[i];
			}
		else
			{
			}
		}
	if (options.datfile_name == NULL)
		{
		fputs("Error: no datfile is specified", stderr);
		return -1;
		}

	return processFile(options.datfile_name);
	}
