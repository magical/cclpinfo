/* cclpinfo v1.4 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PASSWORD_MASK 0x99
#define CHIP_TILE 0x02
#define LAYER_SIZE (32 * 32)

struct file
{
	FILE *fp;
	const char *name;
	int pos; // absolute position
	int sz; // number of bytes left to read in this section
	int err; // 1=end of section; 2=end of file or read error
};

struct levelinfo
{
	int number;
	int time;
	int chips;
	int totalchips;
	char *title;
	char *password;
	char *hint;
	int upperlayersize;
	int lowerlayersize;
};

struct
{
	bool showpassword;
	bool showtime;
	bool showhint;
	bool showchips;
} options;

void warnf(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	//abort();
}

uint8_t readbyte(struct file *file)
{
	uint8_t buf[1] = {};
	if (file->err) {
		return 0;
	}
	if (file->sz < 1 && file->sz > 0) {
		file->err = 1;
		return 0;
	}
	if (fread(buf, sizeof buf, 1, file->fp) != 1) {
		file->err = 2;
		return 0;
	}
	file->pos += 1;
	file->sz -= 1;
	return buf[0];
}

uint16_t readword(struct file *file)
{
	uint8_t buf[2] = {};
	if (file->err) {
		return 0;
	}
	if (file->sz < 2 && file->sz > 0) {
		file->err = 1;
		return 0;
	}
	if (fread(buf, sizeof buf, 1, file->fp) != 1) {
		file->err = 2;
		return 0;
	}
	file->pos += 2;
	file->sz -= 2;
	return ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
}

void readstring(struct file *file, int len, char **bufp, char *debug_name)
{
	char *buf;
	ssize_t n;

	// XXX limit to file->sz
	buf = *bufp = realloc(*bufp, len + 1);
	n = fread(buf, 1, len, file->fp);
	file->pos += n;
	file->sz -= n;
	if (n != len) {
		file->err = 2;
	} else if (n > 0 && buf[n - 1] != '\0') {
		// note: we only issue this warning if the whole field was
		// successfully read, since a field that was cut off in the
		// middle is unlikely to be terminated correctly
		warnf("%s:%d: warning: %s not null terminated\n", file->name, file->pos, debug_name);
	}
	buf[n] = '\0';
}

void skipbytes(struct file *file, int n)
{
	if (fseek(file->fp, n, SEEK_CUR) == 0) {
		file->sz -= n;
		file->pos += n;
	} else {
		//perror("fseek");
		// unseekable file
		for (; n > 0; n--) {
			if (file->sz <= 0) {
				file->err = 1;
				break;
			}
			if (fgetc(file->fp) == EOF) {
				file->err = 2;
				break;
			}
			file->pos += 1;
			file->sz -= 1;
		}
	}
}


int count_tiles(struct file *file, int layersize, int search_tile, char *layer_name)
{
	int total = 0;
	int count = 0;
	int c, tile;

	int start = file->pos;

	int oldsz = file->sz;
	file->sz = layersize;
	// XXX what if layersize > oldsz

	while (file->sz > 0 && file->err == 0) {
		c = readbyte(file);
		if (c == 0xff) {
			c = readbyte(file);
			tile = readbyte(file);
			if (tile == search_tile) {
				count += c;
			}
			total += c;
		} else {
			if (c == search_tile) {
				count += 1;
			}
			total += 1;
		}
	}

	file->sz = oldsz - (layersize - file->sz);

	if (total != LAYER_SIZE) {
		warnf("%s:%d: warning: %s layer has %d tiles, expected %d\n", file->name, start, layer_name, total, LAYER_SIZE);
	}

	return count;
}

void readlevel(struct file *file, off_t levelsize, struct levelinfo *info)
{
	int miscsize;
	int i;

	int start = file->pos;
	file->sz = levelsize;

	info->number = readword(file); // the level number
	info->time = readword(file);   // the time limit
	info->chips = readword(file);  // # chips required
	readword(file);                // 0x0001

	if (file->err) {
		goto err;
	}

	info->totalchips = 0;
	info->upperlayersize = readword(file); // length of upper layer data
	info->totalchips += count_tiles(file, info->upperlayersize, CHIP_TILE, "upper");
	info->lowerlayersize = readword(file); // length of lower layer data
	info->totalchips += count_tiles(file, info->lowerlayersize, CHIP_TILE, "lower");

	miscsize = readword(file); // length of misc data

	if (miscsize != file->sz) {
		warnf("%s:%d: warning: mismatch between level size and misc data size\n", file->name, file->pos-2);
	}

	if (miscsize > 1152) {
		warnf("%s:%d: warning: misc data size exceeds 1152 bytes; MSCC will not be able to load this level\n", file->name, file->pos-2);
	}

	while (file->sz > 0) {
		int fieldnum, fieldlength;
		fieldnum = readbyte(file);
		fieldlength = readbyte(file);
		if (file->err) {
			goto err;
		}
		switch (fieldnum) {
		// 1: time limit
		// 2: chips
		case 3: // level title
			readstring(file, fieldlength, &info->title, "level title");
			break;
		// 4: trap connections
		// 5: clone connections
		case 6: // password
			readstring(file, fieldlength, &info->password, "password");
			// XXX warn if password isn't ascii?
			for (i = 0; i < fieldlength && info->password[i] != '\0'; i++) {
				info->password[i] ^= PASSWORD_MASK;
			}
			break;
		case 7: // level hint
			readstring(file, fieldlength, &info->hint, "hint");
			break;
		// I don't know what 8 and 9 are for
		// 10: creature list
		default:
			skipbytes(file, fieldlength);
		}
	}

	// XXX warn about duplicates too
	if (info->title == NULL) {
		warnf("%s:%d: warning: level has no title\n", file->name, start);
	}
	if (info->password == NULL) {
		warnf("%s:%d: warning: level has no password\n", file->name, start);
	}

	if (!feof(file->fp) && file->pos != ftell(file->fp)) {
		warnf("pos %d != %d\n", file->pos, ftell(file->fp));
		abort();
	}
	return;

err:
	if (file->err == 1) {
		warnf("%s:%d: unexpected end of level data\n", file->name, file->pos);
		// or some other field?
	} else if (feof(file->fp)) {
		warnf("%s:%d: unexpected end of file\n", file->name, file->pos);
	} else if (ferror(file->fp)) {
		warnf("%s:%d: error reading file: %s", file->name, file->pos, strerror(errno));
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
		if (*path == '/' || *path == '\\') {
			filename = path + 1;
		}
	}
	return filename;
}

int processFile(const char *filename)
{
	FILE *fp = NULL;
	struct file file = {};
	struct levelinfo info = {};
	int nLevels, i;
	uint16_t signature, version;

	fp = fopen(filename, "rb");
	if (!fp) {
		warnf("%s: unable to open: %s\n", filename, strerror(errno));
		return 1;
	}

	file.fp = fp;
	file.name = filename;
	file.pos = 0;
	file.sz = -1;

	signature = readword(&file);
	version = readword(&file);
	if (signature != 0xaaac || (version != 0x0002 && version != 0x0102 && version != 0x0003)) {
		warnf("%s: error: not a valid Chip's Challenge file\n", filename);
		fclose(fp);
		return 1;
	}

	nLevels = readword(&file);

	printf("%s\n\n", extract_filename(filename));

	printf("  #. %-35s%s%s%s\n", "Title",
	       (options.showpassword ? "\tPass" : ""),
	       (options.showtime ? "\tTime" : ""),
	       (options.showchips ? "\tChips" : ""));

	for (i = 1; i <= nLevels; i++) {
		file.sz = -1;
		int levelsize = readword(&file);
		if (levelsize == 0) {
			// XXX warn?
			break;
		}
		readlevel(&file, levelsize, &info);
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

	// parse the command-line options
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
