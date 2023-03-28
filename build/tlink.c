/*
** tlink -- Make a symlinked copy of a tree.
** mebbe this should have been a perl script or a shell script,
** but it's too late now!
*/

#include	<ftw.h>
#include	<stdlib.h>
#include	<sys/time.h>
#include	<sys/resource.h>
#include	<stdio.h>

int exit_status;
int keep_going;
int verbose;

char *src_root, *dst_root;

void
usage()
{
  fprintf(stderr, "usgae: tlink [ -kv ] existing new\n");
  fprintf(stderr, "              -k	Keep going after errors\n");
  fprintf(stderr, "              -v	Verbose\n");
  exit(2);
}


int
one(const char *src, const struct stat *file, int type)
{
  char dst[8192];

  strcpy(dst, dst_root);
  strcat(dst, src + strlen(src_root));
  switch (type) 
  {
  case FTW_F:
    if (verbose)
      printf("> %s\n", dst);
    if (symlink(src, dst) < 0) 
    {
      perror(dst);
      if (keep_going == 0)
	exit(1);
      exit_status = 1;
    }
    return 0;
  case FTW_D:
  {
    struct stat dstat;
    if (verbose)
      printf(">> %s\n", dst);
    
    if (stat(dst, &dstat) < 0) 
    {
      if (mkdir (dst, 0777) < 0) 
      {
	perror(dst);
	if (keep_going == 0)
	  exit(1);
	exit_status = 1;
      }
    } else {
      if ((dstat.st_mode & S_IFDIR) == 0)
      {
	fprintf(stderr, "%s: Not a directory\n", dst);
	if (keep_going == 0)
	  exit(1);
	exit_status = 1;
      }
    }
    return 0;
  }
  default:
    return 1;
  }
}
    
void
tlink(char *src, char *dst)
{
    struct rlimit lim;

    getrlimit(RLIMIT_NOFILE, &lim);
	 
    src_root = src;
    dst_root = dst;
    ftw(src, one, lim.rlim_cur - 50);
}

void
main(int argc, char **argv)
{
  int o;

  while ((o = getopt(argc, argv, "vk")) != EOF) {
    switch (o)
    {
    case 'k':
      keep_going = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      usage();
    }
  }	
  if (optind+2 != argc)
    usage();
  tlink(argv[optind], argv[optind+1]);
  exit(exit_status);
}
