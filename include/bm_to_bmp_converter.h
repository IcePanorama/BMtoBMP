#ifndef _BM_TO_BITMAP_CONVERTER_H_
#define _BM_TO_BITMAP_CONVERTER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int8_t
read_uint32_from_file (FILE *fptr, uint32_t *output)
{
  uint8_t bytes[4];
  size_t bytes_read = fread (bytes, sizeof (uint8_t), 4, fptr);
  if (bytes_read != 4)
    {
      fprintf (stderr, "[BMtoBMP] fread error: read %zu bytes, expected %d.\n",
               bytes_read, 4);
      return -1;
    }

  *output = ((uint32_t)bytes[3] << 24) | ((uint32_t)bytes[2] << 16)
            | ((uint32_t)bytes[1] << 8) | (uint32_t)bytes[0];

  return 0;
}

// 24-bpp
#define BYTES_PER_PIXEL (3)
int8_t
BMtoBMP_convert_image (FILE *bm_file,
                       FILE *pal_file) //, const char *output_filename)
{
  uint32_t image_width;
  uint32_t image_height;
  if (read_uint32_from_file (bm_file, &image_width) != 0
      || read_uint32_from_file (bm_file, &image_height) != 0)
    {
      return -1;
    }

  printf ("width: %d, height: %d\n", image_width, image_height);

  uint8_t *image_buffer = calloc (image_width * image_height * BYTES_PER_PIXEL,
                                  sizeof (uint8_t));
  if (image_buffer == NULL)
    {
      fprintf (stderr,
               "[BMtoBMP] calloc error: unable to allocate image buffer of "
               "size, %dx%dx%d.\n",
               image_height, image_width, BYTES_PER_PIXEL * 8);
      goto clean_up;
    }

  for (uint32_t i = 0; i < image_width; i++)
    {
      for (uint32_t j = 0; j < image_height; j++)
        {
          /* Get PAL offset from BM file. */
          uint8_t offset;
          size_t bytes_read = fread (&offset, sizeof (uint8_t), 1, bm_file);
          if (bytes_read != 1)
            {
              fprintf (
                  stderr,
                  "[BMtoBMP] fread error: error reading data from BM file.\n");
              goto clean_up;
            }

          /* Read color data from PAL file using offset. */
          fseek (pal_file, offset, SEEK_SET);
          uint8_t color_data[3] = { 0 };
          for (uint8_t k = 0; k < 3; k++)
            {
              bytes_read
                  = fread (&color_data[k], sizeof (uint8_t), 1, pal_file);
              if (bytes_read != 1)
                {
                  fprintf (stderr, "[BMtoBMP] fread error: error reading data "
                                   "from PAL file.\n");
                  goto clean_up;
                }
            }

          image_buffer[(i * image_height + j) * 3] = color_data[0];
          image_buffer[(i * image_height + j) * 3 + 1] = color_data[1];
          image_buffer[(i * image_height + j) * 3 + 2] = color_data[2];
        }
    }

  free (image_buffer);
  return 0;
clean_up:
  free (image_buffer);
  return -1;
}

#endif /* _BM_TO_BITMAP_CONVERTER_H_ */
