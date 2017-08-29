cclpinfo (Chip's Challenge level pack info)     version 1.4

cclpinfo reads a Chip's Challenge file and displays the level names and
numbers. It can optionally display the time limit, password, chips required,
and level hints as well.

Usage:  cclpinfo [-ptch] files...

    files   one or more levelsets to read
    -p      show passwords
    -t      show time limits
    -c      show chips required/available
    -h      show hints

When the -c option is used, if the number of required chips is the
same as the number of available chips, only one number is displayed;
but if they are different, they are both displayed.

For example:

    103. Island Hopping                             30
    104. Pyramid                                    66/84

Island Hopping from the original levelset requires 30 chips and has 30 chips
available. Pyramid has 84 chips available but only 66 are required to open the
chip socket.

If the file is not a valid Chip's Challenge file, cclpinfo will abort. 
Whether or not the file is a valid Chip's Challenge file is determined 
by looking for a "signature" at the beginning of the file. The name of 
the file does not matter.

Tip: If you want to save the output to a file, add " > filename" to the end
of the line. Be careful: if the file already exists, the contents will be 
erased. To append to an existing file, add " >> filename" instead.

Example:

    cclpinfo CHIPS.dat -pt > chipsinfo.txt

This will make (or overwrite) a file called "chipsinfo.txt" in the current 
directory, and into the file will be written a list of numbers, titles, 
passwords, and time limits for every level in "CHIPS.dat"

-----------------------------------------------------------------------

cclpinfo was written by Andrew E. It is based off a program called datstat
which was written by Madhav Shanbhag and posted to the Chip's Challenge
newsgroup.

You can find the latest version of cclpinfo at 
https://github.com/magical/cclpinfo/
or
https://tilde.town/~magical/chip/#cclpinfo

Please report bugs on github
https://github.com/magical/cclpinfo/issues/new

The windows executable was compiled with MinGW gcc v6.3.0 on Fedora 25.
