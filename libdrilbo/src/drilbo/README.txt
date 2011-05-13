

 Drilbo (Drilbo Represents an Imaging Libary not only for Blorb Objects) is
 the imaging support library for the fizmo interpreter. It supports the
 following file input formats:
  - JPEG
  - PNG
  - Z-Machine V6 graphics, MG1 format

 Not supported
  - Z-Machine V6 graphics, Amiga and Macintosh format

 The following file output formats are supported:
   - JPEG  in RGB, Grayscale and JCS_YCbCr.
   - PPM (for testing purposes)

 The following screen output methods are supported:
   - X11 display though XLib

 These imaging operations are implemented:
   - Bilinear scaling

 All operations use a "z_image" type which holds all the data and metadata
 for images. A z_image may contain either an RGB or a grayscale file with
 a depth of 8 bit per pixel.
 
 See "drilbo.h" for more information on the specific functions.




 From The Z-Machine-Specification, section 8 (just for reference):
 ------------------------------------------------------------------------
  Some details of the known IBM graphics files are given in Paul David
  Doherty's "Infocom Fact Sheet". See also Mark Howell's program
  "pix2gif", which extracts pictures to GIF files. (This is one of his
  "Ztools" programs.)

  Although Version 6 graphics files are not specified here, and were
  released in several different formats by Infocom for different
  computers, a consensus seems to have emerged that the MCGA pictures are
  the ones to adopt (files with filenames *.MG1). These are visually
  identical to Amiga pictures (whose format has been deciphered by Mark
  Knibbs). However, some Version 6 story files were tailored to the
  interpreters they would run on, and use the pictures differently
  according to what they expect the pictures to be. (For instance, an
  Amiga-intended story file will use one big Amiga-format picture where
  an MSDOS-intended story file will use several smaller MCGA ones.)

  The easiest option is to interpret only DOS-intended Version 6 story
  files and only MCGA pictures. But it may be helpful to examine the
  Frotz source code, as Frotz implements draw_picture and picture_data so
  that Amiga and Macintosh forms of Version 6 story files can also be
  used.

  It is generally felt that newly-written graphical games should not
  imitate the old Infocom graphics formats, which are very awkward to
  construct and have been overtaken by technology. Instead, the draft
  Blorb proposal for packaging up resources with Z-machine games calls
  for PNG format graphics glued together in a fairly simple way. An ideal
  Version 6 interpreter ought to understand both the four Infocom
  picture-sets and any Blorb set, thus catering for old and new
  games alike.
 ------------------------------------------------------------------------

