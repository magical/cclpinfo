/* cclpinfo v1.4 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSWORD_MASK 0x99
#define CHIP_TILE 0x02

// This is used when seperating the filename from the path.
// When compiling for *nix, it should be changed to '/'
#define DIR_SEPERATOR '/'

typedef unsigned char byte;

struct levelinfo {
	int number;
	int time;
	int chips;
	int totalchips;
	char *title;
	char *password;
	char *hint;
	int upperlayersize;
	byte *upperlayer;
	int lowerlayersize;
	byte *lowerlayer;
};

struct
{
	bool showpassword;
	bool showtime;
	bool showhint;
	bool showchips;
} options;

uint8_t readbyte(FILE *fp)
{
	uint8_t buf[1] = {};
	fread(buf, sizeof buf, 1, fp);
	return buf[0];
}

uint16_t readword(FILE *fp)
{
	uint8_t buf[2] = {};
	if (fread(buf, sizeof buf, 1, fp) != 1) {
		return 0;
	}
	return ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
}

char *readstring(FILE *fp, int len)
{
	char *buf = malloc(len + 1);
	ssize_t n = fread(buf, 1, len, fp);
	if (n > 0 && buf[n - 1] != '\0') {
		fprintf(stderr, "warning: string not null terminated\n");
	}
	buf[n] = '\0';
	return buf;
}

void skipbytes(FILE *fp, int n)
{
	if (fseek(fp, n, SEEK_CUR) < 0) {
		while (n--) {
			fgetc(fp);
		}
		//perror("fseek");
	}
}

int count_tiles(FILE *fp, int layersize, int search_tile)
{
	int count = 0;
	int c, tile;

	while (layersize > 0) {
		c = fgetc(fp);
		if (c == EOF) {
			warn("unexpected end of file");
			break;
		}
		if (c == 0xff) {
			c = fgetc(fp);
			tile = fgetc(fp);
			if (tile == search_tile) {
				count += c;
			}
			layersize -= 3;
		} else {
			if (c == search_tile) {
				count += 1;
			}
			layersize -= 1;
		}
	}
	return count;
}

void readlevel(FILE *fp, off_t levelsize, struct levelinfo *info)
{
	int l = levelsize;
	int miscsize;
	int i;

	info->number = readword(fp); // the level number
	info->time = readword(fp);   // the time limit
	info->chips = readword(fp);  // # chips required
	readword(fp);                // 0x0001
	l -= 2 * 4;

	info->totalchips = 0;
	info->upperlayersize = readword(fp); // length of upper layer data
	info->totalchips += count_tiles(fp, info->upperlayersize, CHIP_TILE);
	info->lowerlayersize = readword(fp); // length of lower layer data
	info->totalchips += count_tiles(fp, info->lowerlayersize, CHIP_TILE);

	l -= 2 + info->upperlayersize;
	l -= 2 + info->lowerlayersize;

	miscsize = readword(fp); // length of misc data
	l -= 2;


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
			skipbytes(fp, fieldlength);
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
	if (options.showpassword) {
		fprintf(out, "\t%s", info->password);
	}
	if (options.showtime) {
		if (info->time == 0) {
			fprintf(out, "\t---");
		} else {
			fprintf(out, "\t%03d", info->time);
		}
	}
	if (options.showchips) {
		if (info->chips == info->totalchips) {
			fprintf(out, "\t%d", info->chips);
		} else {
			fprintf(out, "\t%d/%d", info->chips, info->totalchips);
		}
	}

	fprintf(out, "\n");

	if (options.showhint && info->hint != NULL) {
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

int processFile(const char *filename)
{
	FILE *fp = NULL;
	struct levelinfo info = {};
	int nLevels, i;
	uint16_t signature, version;

	fp = fopen(filename, "rb");
	if (!fp) {
		printf("Unable to open file: %s\n", filename);
		return 1;
	}

	signature = readword(fp);
	version = readword(fp);
	if (signature != 0xaaac || (version != 0x0002 && version != 0x0102)) {
		fprintf(stderr, "%s is not a valid Chip's Challenge file.\n", filename);
		fclose(fp);
		return 1;
	}

	nLevels = readword(fp);

	printf("%s\n\n", extract_filename(filename));

	printf("  #. %-35s%s%s%s\n", "Title",
	       (options.showpassword ? "\tPass" : ""),
	       (options.showtime ? "\tTime" : ""),
	       (options.showchips ? "\tChips" : ""));

	for (i = 1; i <= nLevels; i++) {
		int levelsize = readword(fp);
		if (levelsize == 0)
			break;
		readlevel(fp, levelsize, &info);
		printlevel(stdout, &info);
		freelevel(&info);
	}

	printf("\n");

	fclose(fp);
	return 0;
}

void usage(const char *program)
{
	puts("cclpinfo v1.4 written by Andrew E. and Madhav Shanbhag");
	puts("");
	printf("Usage:\t%s [-ptch] files...\n", program);
	puts("");
	puts("\tfiles\tone or more levelsets to read");
	puts("\t-p\tshow passwords");
	puts("\t-t\tshow time limits");
	puts("\t-c\tshow chips required/available");
	puts("\t-h\tshow hints");
	puts("");
	puts("For more information, read README.TXT");
}

int main(int argc, const char *argv[])
{
	int i, j;
	int nfiles;
	int err;

	if (argc <= 1) {
		usage(argv[0]);
		return 2;
	}

	// initialize the options
	options.showpassword = false;
	options.showtime = false;
	options.showchips = false;
	options.showhint = false;

	//parse the command-line options
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j] != '\0'; j++) {
				switch (argv[i][j]) {
				case 'p':
					options.showpassword = true;
					break;
				case 't':
					options.showtime = true;
					break;
				case 'c':
					options.showchips = true;
					break;
				case 'h':
					options.showhint = true;
					break;
				default:
					fprintf(stderr, "Warning: unknown option -%c\n", argv[i][j]);
				}
			}
		}
	}

	err = 0;
	nfiles = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			nfiles++;
			if (processFile(argv[i]) != 0) {
				err = 1;
			}
		}
	}

	if (nfiles == 0) {
		fprintf(stderr, "error: no files specified");
		return 2;
	}

	return err;
}
