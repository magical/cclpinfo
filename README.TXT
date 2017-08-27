cclpinfo (Chip's Challenge level pack info)
version 1.2

Program usage:	cclpinfo file [-ptch]

Reads a Chip's Challenge file specified by datfile, and displays the level 
names and numbers. You can have it display time limit, password, chips 
required, and level hints with command-line options.

	-p	display the level passwords
	-t	display the level time limits
	-c	display the chips required to open the chip socket
	-h	display the level hints

If the file is not a valid Chip's Challenge file, cclpinfo will abort. 
Whether or not the file is a valid Chip's Challenge file is determined 
by looking for a "signature" at the beginning of the file. The name of 
the file does not matter.

If you want to save the output to a file, add " > filename" to the end of 
the line. Be careful: if the file already exists, the contents will be 
erased. To append to an existing file, add " >> filename" instead.

Example:

cclpinfo CHIPS.dat -pt > chipsinfo.txt

This will make (or overwrite) a file called "chipsinfo.txt" in the current 
directory, and into the file will be written a list of numbers, titles, 
passwords, and time limits for every level in "CHIPS.dat"

The original program, called "datstat", was written by Madhav Shanbhag and 
posted to the Chip's Challenge newsgroup (annexcafe.chips.challenge) at 
news.annexcafe.com. It was modified by me, Andrew E.

You can find the latest version of cclpinfo at 
http://a_magical_me.home.comcast.net/chip/chip.html

The executable included in this zip file was compiled with MinGW gcc v3.4.2 
on Windows XP SP2. The zip file was created using 7-zip.