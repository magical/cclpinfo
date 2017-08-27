cclpinfo: cclpinfo.c
	$(CC) -O2 -Wall -o cclpinfo cclpinfo.c

cclpinfo.exe: cclpinfo.c
	i686-w64-mingw32-gcc -O2 -o $@ cclpinfo.c
	i686-w64-mingw32-strip -s $@

release: cclpinfo.exe README.TXT CHANGELOG.TXT cclpinfo.c
	rm -f cclpinfo.zip
	zip cclpinfo.zip README.TXT CHANGELOG.TXT cclpinfo.c cclpinfo.exe
