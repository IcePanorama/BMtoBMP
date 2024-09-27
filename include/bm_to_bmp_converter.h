#ifndef _BM_TO_BITMAP_CONVERTER_H_
#define _BM_TO_BITMAP_CONVERTER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BMtoBMP_BYTES_PER_PIXEL (3) // 24-bits-per-pixel
#define BMtoBMP_OUTPUT_FILENAME_MAX_LEN (256)
#define BMtoBMP_DPI (96)

typedef struct BMtoBMP_BitmapImage_s
{
  char filename[BMtoBMP_OUTPUT_FILENAME_MAX_LEN];
  uint32_t width;
  uint32_t height;
  uint8_t **data;
} BMtoBMP_BitmapImage_t;

static void
allocate_img_data (BMtoBMP_BitmapImage_t *img)
{
  img->data = (uint8_t **)calloc (img->height, sizeof (uint8_t *));
  if (img->data == NULL)
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr,
               "[BMtoBMP] calloc error: unable to allocate image buffer of "
               "size, %dx%dx%d.\n",
               img->height, img->width, BMtoBMP_BYTES_PER_PIXEL * 8);
#endif /* BMtoBMP_DEBUG_OUTPUT */
      return;
    }

  for (uint32_t i = 0; i < img->height; i++)
    {
      img->data[i] = (uint8_t *)calloc (img->width * BMtoBMP_BYTES_PER_PIXEL,
                                        sizeof (uint8_t));
      if (img->data[i] == NULL)
        {
          for (uint32_t j = 0; j < i; j++)
            {
              free (img->data[j]);
            }
          free (img->data);
          img->data = NULL;
          return;
        }
    }
}

static void
destroy_img_data (BMtoBMP_BitmapImage_t *img)
{
  if (img == NULL)
    return;

  if (img->data != NULL)
    {
      for (uint32_t i = 0; i < img->height; i++)
        {
          free (img->data[i]);
        }
      free (img->data);
      img->data = NULL;
    }
}

static int8_t
read_uint32_from_file (FILE *fptr, uint32_t *output)
{
  uint8_t bytes[4];
  size_t bytes_read = fread (bytes, sizeof (uint8_t), 4, fptr);
  if (bytes_read != 4)
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr, "[BMtoBMP] fread error: read %zu bytes, expected %d.\n",
               bytes_read, 4);
#endif /* BMtoBMP_DEBUG_OUTPUT */
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
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write le int32 to file.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
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
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write le uint16 to file.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
      return -1;
    }

  return 0;
}

static int8_t
write_le_int8_to_file (FILE *fptr, uint8_t x)
{
  if (fwrite (&x, sizeof (uint8_t), sizeof (x), fptr) != sizeof (x))
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write le uint16 to file.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
      return -1;
    }

  return 0;
}

static int8_t
write_string_to_file (FILE *fptr, const char *s, size_t s_len)
{
  if (fwrite (s, sizeof (char), s_len, fptr) != s_len)
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr,
               "[BMtoBMP] fwrite error: failed to write string to file.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
      return -1;
    }
  return 0;
}

static int8_t
process_image (FILE *bm_file, FILE *pal_file, BMtoBMP_BitmapImage_t *img)
{
  fseek (bm_file, 0x0C, SEEK_SET);

  // for (uint32_t i = 0; i < img->height; i++)
  for (int32_t i = img->height - 1; i >= 0; i--)
    {
      for (uint32_t j = 0; j < img->width; j++)
        {
          /* Get PAL offset from BM file. */
          uint8_t offset;
          size_t bytes_read = fread (&offset, sizeof (uint8_t), 1, bm_file);
          if (bytes_read != 1)
            {
#ifdef BMtoBMP_DEBUG_OUTPUT
              fprintf (
                  stderr,
                  "[BMtoBMP] fread error: error reading data from BM file.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
              return -1;
            }

          /* Read color data from PAL file using offset. */
          fseek (pal_file, offset * 0x3, SEEK_SET);
          uint8_t color_data[3] = { 0 };
          for (uint8_t k = 0; k < 3; k++)
            {
              bytes_read
                  = fread (&color_data[k], sizeof (uint8_t), 1, pal_file);
              if (bytes_read != 1)
                {
#ifdef BMtoBMP_DEBUG_OUTPUT
                  fprintf (stderr, "[BMtoBMP] fread error: error reading data "
                                   "from PAL file.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
                  return -1;
                }
            }

          /* Data must be in LE order so it actually goes BGR, not RGB. */
          img->data[i][j * 3] = color_data[2];
          img->data[i][j * 3 + 1] = color_data[1];
          img->data[i][j * 3 + 2] = color_data[0];
        }
    }

  return 0;
}

static int8_t
create_image (FILE *bm_file, BMtoBMP_BitmapImage_t *img)
{
  if (read_uint32_from_file (bm_file, &img->width) != 0
      || read_uint32_from_file (bm_file, &img->height) != 0)
    {
      return -1;
    }

  allocate_img_data (img);
  if (img->data == NULL)
    return -1;

  return 0;
}

int8_t
BMtoBMP_convert_image (FILE *bm_file, FILE *pal_file,
                       char const output_filename[static 1])
{
  /* len(".BMP\0") = 5 */
  if (strlen (output_filename) + 5 > BMtoBMP_OUTPUT_FILENAME_MAX_LEN)
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr, "[BMtoBMP] output filename is too long.\n");
#endif /* BMtoBMP_DEBUG_OUTPUT */
      return -1;
    }

  BMtoBMP_BitmapImage_t img;
  if (create_image (bm_file, &img) != 0)
    return -1;

  if (process_image (bm_file, pal_file, &img) != 0)
    goto clean_up;

  strcpy (img.filename, output_filename);
  strcat (img.filename, ".bmp");

  FILE *output = fopen (img.filename, "wb");
  if (output == NULL)
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr, "[BMtoBMP] fopen error: could not create file, %s.\n",
               img.filename);
#endif /* BMtoBMP_DEBUG_OUTPUT */
      goto clean_up;
    }

  // Make sure we're at the beginning of the file
  fseek (output, 0x0, SEEK_SET);

  const uint32_t pixel_data_size
      = (img.width * img.height * BMtoBMP_BYTES_PER_PIXEL);
  const uint32_t output_file_size = 54 + pixel_data_size;
  const int32_t ppm_resolution = 0x0B13; // pixel per meter

  /* Populate bitmap file header. (BITMAPINFOHEADER) */
  if (write_string_to_file (output, "BM", 2) != 0 // file signature
      || write_le_int32_to_file (output, output_file_size) != 0
      || write_le_int32_to_file (output, 0x0) != 0  // Reserved
      || write_le_int32_to_file (output, 0x36) != 0 // pixel array offset
      || write_le_int32_to_file (output, 0x28) != 0 // DIB header size
      || write_le_int32_to_file (output, img.width) != 0
      || write_le_int32_to_file (output, img.height) != 0
      || write_le_int16_to_file (output, 0x1) != 0 // num color planes
      || write_le_int16_to_file (output, BMtoBMP_BYTES_PER_PIXEL * 8) != 0
      || write_le_int32_to_file (output, 0x0) != 0 // no compression
      || write_le_int32_to_file (output, 0x0) != 0
      || write_le_int32_to_file (output, ppm_resolution) // horizontal
      || write_le_int32_to_file (output, ppm_resolution) // vertical
      || write_le_int32_to_file (output, 0x0) // num colors in palette
      || write_le_int32_to_file (output, 0x0) // num important colors
  )
    {
      fclose (output);
      goto clean_up;
    }

  /* Output image data to file. */
  const uint32_t padding = (4 - (img.width * BMtoBMP_BYTES_PER_PIXEL) % 4) % 4;
  for (uint32_t i = 0; i < img.height; i++)
    {
      for (uint32_t j = 0; j < img.width; j++)
        {
          for (uint32_t k = 0; k < BMtoBMP_BYTES_PER_PIXEL; k++)
            {
              if (write_le_int8_to_file (output, img.data[i][j * 3 + k]) != 0)
                {
                  fclose (output);
                  goto clean_up;
                }
            }
        }
      for (uint32_t p = 0; p < padding; p++)
        {
          write_le_int8_to_file (output, 0x0);
        }
    }

  /* Update file size in header. */
  size_t size = ftell (output);
  fseek (output, 0x22, SEEK_SET);
  write_le_int32_to_file (output, size - 54);

  destroy_img_data (&img);
  fclose (output);
  return 0;
clean_up:
  destroy_img_data (&img);
  return -1;
}

#endif /* _BM_TO_BITMAP_CONVERTER_H_ */
