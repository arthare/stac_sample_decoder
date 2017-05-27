There's a .sln file in the lzfse directory that compiles the lzfse decoder and the sample stac decoder.

There's a sample .stac file in /simple_stac_decoder called tulipsOnTable.stac.

This has, as of March 22 2017, only been compiled in Microsoft Visual Studio Express 2013 for Windows Desktop.  I tried to make it not-particularly-microsoftey, so it should be easy to compile in other areas.

Usage:
simple_stac_decoder.exe <path to your stac file> [bmp|bin|ply]

Examples:
simple_stac_decoder.exe tulipsOnTable.stac bmp
simple_stac_decoder.exe tulipsOnTable.stac bin
simple_stac_decoder.exe tulipsOnTable.stac ply

Those examples will output all your frames to bitmap files (so you can view them) or binary files (so you can read raw data) or .ply files (so you can use pointclouds in other apps).  The .bin format is simply 640*480 floats, with values representing that pixel's depth in millimeters.

Good luck!
