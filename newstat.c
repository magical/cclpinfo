#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PASSWORD_MASK 0x99

typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned long int dword;


struct
{
	word number;
	word time;
	word chips;
	char *title;
	char *password;
	char *hint;
} levelInfo;

struct
{
	const char *datfile_name;
	bool display_passwords:1;
	bool display_time:1;
	bool display_hints:1;
} options;

void displayLevelStats(FILE* fp)
	{
	int i = 0;
	char *j;
	word length;

	levelInfo.title = NULL;
	levelInfo.password = NULL;
	levelInfo.hint = NULL;

	fread(&levelInfo.number, sizeof(word), 1, fp); // the level number
	fread(&levelInfo.time, sizeof(word), 1, fp); // the time limit
	fread(&levelInfo.chips, sizeof(word), 1, fp); // # chips required
	fseek(fp, sizeof(word), SEEK_CUR); // 0x0001
	fread(&length, sizeof(word), 1, fp); // length of upper layer data
	fseek(fp, length, SEEK_CUR); // we don't care about the level data
	fread(&length, sizeof(word), 1, fp); // length of lower layer data
	fseek(fp, length, SEEK_CUR);
	fread(&length, sizeof(word), 1, fp); // length of misc data

	i = 0;
	while (i < length)
		{
		byte fieldnum, fieldlength;
		fread(&fieldnum, sizeof(byte), 1, fp);
		fread(&fieldlength, sizeof(byte), 1, fp);
		switch (fieldnum)
			{
			// 1 == time limit
			// 2 == chips
			case 3:
				levelInfo.title = malloc(fieldlength);
				fread(levelInfo.title, fieldlength, 1, fp);
				break;
			// 4 == trap connections
			// 5 == clone connections
			case 6:
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
			case 7:
				levelInfo.hint = malloc(fieldlength);
				fread(levelInfo.hint, fieldlength, 1, fp);
				break;
			// 8 and 9 are purposeless
			// 10 == creature list
			default:
				fseek(fp, fieldlength, SEEK_CUR);
			}
		i += fieldlength + 2;
		}
	
	printf("%3d. %-50s", levelInfo.number, levelInfo.title);
	if (options.display_passwords)
		{
		printf("\t%s", levelInfo.password);
		}
	if (options.display_time)
		{
		if (levelInfo.time == 0)
			{
			printf("\t---");
			}
		else
			{
			printf("\t%03i", levelInfo.time);
			}
		}
	fputc('\n', stdout);

	if (options.display_hints && levelInfo.hint != NULL)
		{
		printf("     %s\n", levelInfo.hint);
		}

	if (levelInfo.title != NULL)
		{
		free(levelInfo.title);
		}
	if (levelInfo.password != NULL)
		{
		free(levelInfo.password);
		}
	if (levelInfo.hint != NULL)
		{
		free(levelInfo.hint);
		}
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
	
	printf("  #. %-50s%s%s\n", "Title", (options.display_passwords?"\tPass":""), (options.display_time?"\tTime":""));
	for (l = 1; l <= nLevels; ++l)
		{
		word levelSize = 0;
		fread(&levelSize, sizeof(word), 1, fp);
		if (levelSize == 0) break;
		displayLevelStats(fp);
		}

	fclose(fp);
	return 0;
	}

int main(int argc, const char *argv[])
	{
	int i, j;
	
	if (argc <= 1)
		{
		puts("Usage:\tdatstat datfile [-pth]");
		puts("");
		puts("\tdatfile\tThe location of the .dat file to use");
		puts("\tp\tDisplay passwords");
		puts("\tt\tDisplay the time limits");
		puts("\th\tDisplay the level hints");
		return -1;
		}
	
	options.datfile_name = NULL;
	options.display_passwords = false;
	options.display_time = false;
	options.display_hints = false;
	//parse the command-line options
	for (i = 1; i < argc; i+=1)
		{
		if (argv[i][0] == '-')
			{
			for (j = 1; j < strlen(argv[i]); j+=1)
				{
				switch (argv[i][j])
					{
					case 'p':
						if (options.display_passwords == true)
							{
							fputs("Warning: option -p is used more than once\n", stderr);
							}
						options.display_passwords = true;
						break;
					case 't':
						if (options.display_time == true)
							{
							fputs("Warning: option -t is used more than once\n", stderr);
							}
						options.display_time = true;
						break;
					case 'h':
						if (options.display_hints == true)
							{
							fputs("Warning: option -h is used more than once\n", stderr);
							}
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
