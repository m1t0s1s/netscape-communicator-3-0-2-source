/*
 *  ZIGFILE.H
 * 
 *  Certain constants and structures for
 *  the Phil Katz ZIP archive format.
 *
 */

/* ZIP */

struct ZipLocal
  {
  char signature [4];
  char word [2];
  char bitflag [2];
  char method [2];
  char time [2];
  char date [2];
  char crc32 [4];
  char size [4];
  char orglen [4];
  char filename_len [2];
  char extrafield_len [2];
  };

struct ZipCentral
  {
  char signature [4];
  char version_made_by [2];
  char version [2];
  char bitflag [2];
  char method [2];
  char time [2];
  char date [2];
  char crc32 [4];
  char size [4];
  char orglen [4];
  char filename_len [2];
  char extrafield_len [2];
  char commentfield_len [2];
  char diskstart_number [2];
  char internal_attributes [2];
  char external_attributes [4];
  char localhdr_offset [4];
  };

struct ZipEnd
  {
  char signature [4];
  char disk_nr [2];
  char start_central_dir [2];
  char total_entries_disk [2];
  char total_entries_archive [2];
  char central_dir_size [4];
  char offset_central_dir [4];
  char commentfield_len [2];
  };

#define LSIG 0x04034B50l
#define CSIG 0x02014B50l
#define ESIG 0x06054B50l

/* TAR */

union TarEntry
  {
  struct header
    {
    char filename [100];
    char mode [8];
    char uid [8];
    char gid [8];
    char size [12];
    char time [12];
    char checksum [8];
    char linkflag;
    char linkname [100];
    }
  val;

  char buffer [512];
  };
