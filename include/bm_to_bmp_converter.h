#ifndef _BM_TO_BITMAP_CONVERTER_H_
#define _BM_TO_BITMAP_CONVERTER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_PIXEL (3) // 24 bpp
#define OUTPUT_FILENAME_MAX_LEN (256)
#define DPI (96)

typedef struct BitmapImage_s
{
  uint32_t width;
  uint32_t height;
  uint8_t *data;
} BitmapImage_t;

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

static int8_t
write_le_int32_to_file (FILE *fptr, uint32_t x)
{
  /* clang-format off */
  uint8_t bytes[4] = {
    (uint8_t)(x & 0xFF),
    (uint8_t)((x & 0xFF00) >> 8),
    (uint8_t)((x & 0xFF0000) >> 16),
    (uint8_t)((x & 0xFF000000) >> 24)
  };
  /* clang-format on */

  if (fwrite (&bytes, sizeof (uint8_t), sizeof (bytes), fptr)
      != sizeof (bytes))
    {
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write le int32 to file.\n");
      return -1;
    }

  return 0;
}

static int8_t
write_le_int16_to_file (FILE *fptr, uint16_t x)
{
  /* clang-format off */
  uint8_t bytes[2] = {
    (uint8_t)(x & 0xFF),
    (uint8_t)((x & 0xFF00) >> 8),
  };
  /* clang-format on */

  if (fwrite (&bytes, sizeof (uint8_t), sizeof (bytes), fptr)
      != sizeof (bytes))
    {
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write le uint16 to file.\n");
      return -1;
    }

  return 0;
}

static int8_t
write_le_int8_to_file (FILE *fptr, uint8_t x)
{
  if (fwrite (&x, sizeof (uint8_t), sizeof (x), fptr) != sizeof (x))
    {
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write le uint16 to file.\n");
      return -1;
    }

  return 0;
}

static int8_t
write_string_to_file (FILE *fptr, const char *s, size_t s_len)
{
  if (fwrite (s, sizeof (char), s_len, fptr) != s_len)
    {
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write string to file.\n");
      return -1;
    }
  return 0;
}

static int8_t
process_image (FILE *bm_file, FILE *pal_file, BitmapImage_t *img)
{
  if (read_uint32_from_file (bm_file, &img->width) != 0
      || read_uint32_from_file (bm_file, &img->height) != 0)
    {
      return -1;
    }

  printf ("width: %d, height: %d\n", img->width, img->height);

  img->data
      = calloc (img->width * img->height * BYTES_PER_PIXEL, sizeof (uint8_t));
  if (img->data == NULL)
    {
      fprintf (stderr,
               "[BMtoBMP] calloc error: unable to allocate image buffer of "
               "size, %dx%dx%d.\n",
               img->height, img->width, BYTES_PER_PIXEL * 8);
      goto clean_up;
    }

  for (uint32_t i = 0; i < img->width; i++)
    {
      for (uint32_t j = 0; j < img->height; j++)
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

          img->data[(i * img->height + j) * 3] = color_data[0];
          img->data[(i * img->height + j) * 3 + 1] = color_data[1];
          img->data[(i * img->height + j) * 3 + 2] = color_data[2];
        }
    }

  return 0;
clean_up:
  free (img->data);
  return -1;
}

int8_t
BMtoBMP_convert_image (FILE *bm_file, FILE *pal_file,
                       char const output_filename[static 1])
{
  BitmapImage_t img;
  if (process_image (bm_file, pal_file, &img) != 0)
    return -1;

  if (strlen (output_filename) + strlen (".bmp") + 1 > OUTPUT_FILENAME_MAX_LEN)
    {
      fprintf (stderr, "[BMtoBMP] output filename is too long.\n");
      goto clean_up;
    }

  char filename[OUTPUT_FILENAME_MAX_LEN];
  strcpy (filename, output_filename);
  strcat (filename, ".bmp");

  FILE *output = fopen (filename, "wb");
  if (output == NULL)
    {
      fprintf (stderr, "[BMtoBMP] fopen error: could not create file, %s.\n",
               filename);
      goto clean_up;
    }

  // Make sure we're at the beginning of the file
  fseek (output, 0x0, SEEK_SET);

  uint32_t pixel_data_size = (img.width * img.height * BYTES_PER_PIXEL);
  const uint32_t padding = (4 - (pixel_data_size % 4)) % 4;
  pixel_data_size += padding;
  const uint32_t output_file_size = 54 + pixel_data_size;

  const int32_t ppm_resolution = (int32_t)(DPI * 39.37); // pixel per meter

  if (write_string_to_file (output, "BM", 2) != 0 // file signature
      || write_le_int32_to_file (output, output_file_size) != 0
      || write_le_int32_to_file (output, 0x0) != 0  // Reserved
      || write_le_int32_to_file (output, 0x36) != 0 // pixel array offset
      || write_le_int32_to_file (output, 0x28) != 0 // DIB header size
      || write_le_int32_to_file (output, img.width) != 0
      || write_le_int32_to_file (output, img.height) != 0
      || write_le_int16_to_file (output, 0x1) != 0 // num color planes
      || write_le_int16_to_file (output, BYTES_PER_PIXEL * 8) != 0
      || write_le_int32_to_file (output, 0x0) != 0 // no compression
      || write_le_int32_to_file (output, pixel_data_size) != 0
      || write_le_int32_to_file (output, ppm_resolution) // horizontal
      || write_le_int32_to_file (output, ppm_resolution) // vertical
      || write_le_int32_to_file (output, 0x0) // num colors in palette
      || write_le_int32_to_file (output, 0x0) // num important colors
  )
    {
      fclose (output);
      goto clean_up;
    }

  // Recalculating pixel data size as padding may have been applied.
  for (uint32_t i = 0; i < img.width * img.height * BYTES_PER_PIXEL; i++)
    {
      write_le_int8_to_file (output, img.data[i]);
    }

  fclose (output);
  free (img.data);
  return 0;
clean_up:
  free (img.data);
  return -1;
}

#endif /* _BM_TO_BITMAP_CONVERTER_H_ */
