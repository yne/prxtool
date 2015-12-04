PRXTool
=======
[![Build Status](https://api.travis-ci.org/yne/prxtool.svg)](https://travis-ci.org/yne/prxtool)
[![Coverage](https://codecov.io/github/yne/prxtool/coverage.svg?branch=master)](https://codecov.io/github/yne/prxtool?branch=master)

This is a simple tool to manipulate Sony PSP(tm) PRX files. Prxtool can:

* output an IDC file which can be used with IDA Pro
* output an ELF file
* disassemble PRX files into a pretty printed format

Installation
------------

To compile `prxtool`, run:

    c99 prxtool.c

Testing
-------

The `/tests/` folder contain samples prx from pspsdk and from Sony
(with randomized section content to avoid copyright infringement).

Additional resources
--------------------
The `/res/` folder contain the 5.00 psplibdoc in .xml and .yml format
along with function and instruction list file in .tsv format.

License
-------

TyRaNiD (c) 2k6

PRXTool is licensed under the AFL v2.0. Please read the LICENSE file for further
information.

Thanks to

* Mrbrown for adding autoconfig
* all the other people who contribute to legit PSP dev work.

This is a good companion to libdoc as that provides the XML file used to get
names and such for proto.

