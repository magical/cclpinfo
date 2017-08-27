/* cclpinfo v1.4 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSWORD_MASK 0x99
#define CHIP_TILE (word)0x02

// This is used when seperating the filename from the path.
// When compiling for *nix, it should be changed to '/'
#define DIR_SEPERATOR '/'

typedef unsigned char byte;
typedef uint16_t word;
typedef uint32_t dword;

struct levelinfo {
	word number;
	word time;
	word chips;
	int totalchips;
	char *title;
	char *password;
	char *hint;
	word upperlayersize;
	byte *upperlayer;
	word lowerlayersize;
	byte *lowerlayer;
};

struct
{
	const char *datfile_name;
	const char *savefile_name;
	bool display_passwords;
	bool display_time;
	bool display_hints;
	bool display_chips;
} options;

uint8_t readbyte(FILE *fp)
{
	byte buf[1] = {};
	fread(buf, sizeof buf, 1, fp);
	return buf[0];
}

uint16_t readword(FILE *fp)
{
	byte buf[2] = {};
	if (fread(buf, sizeof buf, 1, fp) != 1) {
		return 0;
	}
	return ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
}

uint32_t readdword(FILE *fp)
{
	byte buf[4] = {};
	if (fread(buf, sizeof buf, 1, fp) != 1) {
		return 0;
	}
	return ((uint32_t)buf[0]) | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

char *readstring(FILE *fp, int len)
{
	char *buf = malloc(len + 1);
	ssize_t n = fread(buf, 1, len, fp);
	buf[n] = '\0';
	return buf;
}

int count_tiles(byte *layerdata, word layersize, byte search_tile)
{
	int count = 0;
	int i = 0;

	while (i < layersize) {
		if (layerdata[i] == search_tile) {
			count += 1;
			i += 1;
		} else if (layerdata[i] == 0xff) {
			if (layerdata[i + 2] == search_tile) {
				count += layerdata[i + 1];
			}
			i += 3;
		} else {
			i += 1;
		}
	}
	return count;
}

void readlevel(FILE *fp, off_t levelsize, struct levelinfo *info)
{
	int l = levelsize;
	word miscsize;
	int i;

	info->number = readword(fp); // the level number
	info->time = readword(fp);   // the time limit
	info->chips = readword(fp);  // # chips required
	readword(fp);                // 0x0001

	info->totalchips = 0;
	if (options.display_chips) {
		info->upperlayersize = readword(fp); // length of upper layer data
		info->upperlayer = malloc(info->upperlayersize);
		fread(info->upperlayer, sizeof(byte), info->upperlayersize, fp);

		info->lowerlayersize = readword(fp); // length of lower layer data
		info->lowerlayer = malloc(info->lowerlayersize);
		fread(info->lowerlayer, sizeof(byte), info->lowerlayersize, fp);

		info->totalchips += count_tiles(info->upperlayer, info->upperlayersize, CHIP_TILE);
		info->totalchips += count_tiles(info->lowerlayer, info->lowerlayersize, CHIP_TILE);
	} else {
		info->upperlayersize = readword(fp); // length of upper layer data
		fseek(fp, info->upperlayersize, SEEK_CUR);
		info->lowerlayersize = readword(fp); // length of lower layer data
		fseek(fp, info->lowerlayersize, SEEK_CUR);
	}
	l -= info->upperlayersize;
	l -= info->lowerlayersize;

	miscsize = readword(fp); // length of misc data
	l -= sizeof(word) * 7;

	while (l > 0) {
		byte fieldnum, fieldlength;
		fieldnum = readbyte(fp);
		fieldlength = readbyte(fp);
		switch (fieldnum) {
		// 1: time limit
		// 2: chips
		case 3: // level title
			info->title = readstring(fp, fieldlength);
			break;
		// 4: trap connections
		// 5: clone connections
		case 6: // password
			info->password = readstring(fp, fieldlength);
			for (i = 0; i < fieldlength && info->password[i] != '\0'; i++) {
				info->password[i] ^= PASSWORD_MASK;
			}
			break;
		case 7: // level hint
			info->hint = readstring(fp, fieldlength);
			break;
		// I don't know what 8 and 9 are for
		// 10: creature list
		default:
			fseek(fp, fieldlength, SEEK_CUR);
		}
		l -= fieldlength + 2;
	}
}

void freelevel(struct levelinfo *info)
{
	free(info->title);
	free(info->password);
	free(info->hint);

	info->title = NULL;
	info->password = NULL;
	info->hint = NULL;
}

void printlevel(FILE *out, struct levelinfo *info)
{
	fprintf(out, "%3d. %-35s ", info->number, info->title);
	if (options.display_passwords) {
		fprintf(out, "\t%s", info->password);
	}
	if (options.display_time) {
		if (info->time == 0) {
			fprintf(out, "\t---");
		} else {
			fprintf(out, "\t%03d", info->time);
		}
	}
	if (options.display_chips) {
		if (info->chips == info->totalchips) {
			fprintf(out, "\t%d", info->chips);
		} else {
			fprintf(out, "\t%d/%d", info->chips, info->totalchips);
		}
	}

	fprintf(out, "\n");

	if (options.display_hints && info->hint != NULL) {
		fprintf(out, "     %s\n", info->hint);
	}
}

const char *extract_filename(const char *path)
{
	const char *filename = path;
	for (; *path; path++) {
		if (*path == DIR_SEPERATOR) {
			filename = path + 1;
		}
	}
	return filename;
}

int processFile(const char *szDatFileName)
{
	FILE *fp = NULL;
	struct levelinfo info = {};
	word nLevels = 0, l;
	uint32_t signature;

	fp = fopen(szDatFileName, "rb");
	if (!fp) {
		printf("Unable to open file: %s\n", szDatFileName);
		return 1;
	}

	signature = readdword(fp);
	if (signature != 0x0002aaac && signature != 0x0102aaac) {
		fprintf(stderr, "%s is not a valid Chip's Challenge file.\n", szDatFileName);
		fclose(fp);
		return 1;
	}

	nLevels = readword(fp);

	printf("%s\n\n", extract_filename(szDatFileName));

	printf("  #. %-35s%s%s%s\n", "Title",
	       (options.display_passwords ? "\tPass" : ""),
	       (options.display_time ? "\tTime" : ""),
	       (options.display_chips ? "\tChips" : ""));

	for (l = 1; l <= nLevels; ++l) {
		word levelsize = readword(fp);
		if (levelsize == 0)
			break;
		readlevel(fp, levelsize, &info);
		printlevel(stdout, &info);
		freelevel(&info);
	}

	fclose(fp);
	return 0;
}

int main(int argc, const char *argv[])
{
	int i, j;

	if (argc <= 1) {
		puts("cclpinfo v1.4 written by Andrew E. and Madhav Shanbhag");
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
		return 2;
	}

	// initialize the options
	options.datfile_name = NULL;
	options.savefile_name = NULL;
	options.display_passwords = false;
	options.display_time = false;
	options.display_chips = false;
	options.display_hints = false;

	//parse the command-line options
	for (i = 1; i < argc; i += 1) {
		if (argv[i][0] == '-') {
			for (j = 1; j < strlen(argv[i]); j += 1) {
				switch (argv[i][j]) {
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
		} else if (options.datfile_name == NULL) {
			options.datfile_name = argv[i];
		}
	}
	if (options.datfile_name == NULL) {
		fprintf(stderr, "Error: no datfile is specified");
		return -1;
	}

	return processFile(options.datfile_name);
}
