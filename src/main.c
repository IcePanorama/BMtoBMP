#include <stdio.h>
#include <stdlib.h>

FILE *load_file (const char *filename);

int
main (void)
{
  const char *bm_filename = "AUTOGRPH.BM";
  const char *pal_filename = "AUTOGRPH.PAL";

  FILE *bm_file = load_file (bm_filename);
  FILE *pal_file = load_file (pal_filename);

  fclose (bm_file);
  fclose (pal_file);

  return 0;
}

FILE *
load_file (const char *filename)
{
  FILE *fptr = fopen (filename, "rb");
  if (fptr == NULL)
    {
      fprintf (stderr, "Error: unable to open file, %s.\n", filename);
      exit (1);
    }

  return fptr;
}
