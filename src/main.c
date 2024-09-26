#include "bm_to_bmp_converter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *load_file (const char *filename);
static void handle_improper_usage_error (const char *exe_name);
static int8_t validate_user_input (const char *bm_filename,
                                   const char *pal_filename);

int
main (int argc, char **argv)
{
  if ((argc < 3) || validate_user_input (argv[1], argv[2]) != 0)
    handle_improper_usage_error (argv[0]);

  FILE *bm_file = load_file (argv[1]);
  FILE *pal_file = load_file (argv[2]);

  printf ("Converting image, %s.\n", argv[1]);
  if (BMtoBMP_convert_image (bm_file, pal_file, "output") != 0)
    {
      fclose (bm_file);
      fclose (pal_file);
      exit (1);
    }
  puts ("Done!");

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

void
handle_improper_usage_error (const char *exe_name)
{
  fprintf (stderr,
           "Improper usage.\n\ttry: %s path/to/file.BM path/to/file.PAL\n",
           exe_name);
  exit (1);
}

int8_t
validate_user_input (const char *bm_filename, const char *pal_filename)
{
  size_t len = strlen (bm_filename);
  if (strcmp (bm_filename + (len - 3), ".BM") != 0
      && strcmp (bm_filename + (len - 3), ".bm") != 0)
    {
      fprintf (stderr, "Error: %s is not a BM file.\n", bm_filename);
      return -1;
    }

  len = strlen (pal_filename);
  if (strcmp (pal_filename + (len - 4), ".PAL") != 0
      && strcmp (pal_filename + (len - 4), ".pal") != 0)
    {
      fprintf (stderr, "Error: %s is not a BM file.\n", pal_filename);
      return -1;
    }

  return 0;
}
