//  Copyright (C) 2024  IcePanorama
//
//  BMtoBMP is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by the
//  Free Software Foundation, either version 3 of the License, or (at your
//  option) any later version.
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program.  If not, see <https://www.gnu.org/licenses/>.
/* clang-format off */
/**
 *
 *  BMtoBMP - A header-only library for converting BM image files to BMP format
 *  using PAL files.
 *
 *  Debug error output messages can be enabled by compiling with the
 *  `-DBMtoBMP_DEBUG_OUTPUT` flag.
 *
 *  Overview:
 *  This library provides functionality to convert BM image files to standard
 *  24-bit BMP images, using an accompanying PAL file to map pixel values to
 *  RGB colors. This library only exposes one public function for use:
 *  `BMtoBMP_convert_image()`.
 *
 *  Function:
 *  `BMtoBMP_convert_image(FILE *bm_file, FILE *pal_file, const char *output_filename)`
 *    The primary function for converting a BM image file to BMP format.
 *
 *    Input:
 *    `bm_file`: A pointer to an open BM file (`FILE *`), which contains the
 *              image's pixel data, where each byte is an index into the
 *              palette, (`pal_file`).
 *    `pal_file`: A pointer to an open PAL file (`FILE *`), used to map the 
 *              indices in the BM file to RGB color values.
 *    `output_filename`: A null-terminated C string representing the desired
 *              output filename (without extension). The filename must not
 *              exceed 250 characters. This function automatically appends the
 *              ".bmp" extension to the provided name.
 *
 *    Returns:
 *      A zero on success, non-zero on failure.
 *
 *  Responsibility:
 *  - The caller is responsible for opening and closing both input files
 *    (`bm_file` and `pal_file`).
 *  - It is also the caller's responsibility to ensure that the files provided
 *    are valid BM/PAL files, as the library does not perform any validation.
 */
/* clang-format on */
#ifndef _BM_TO_BITMAP_CONVERTER_H_
#define _BM_TO_BITMAP_CONVERTER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BMtoBMP_BYTES_PER_PIXEL (3) // 24-bits-per-pixel
#define BMtoBMP_OUTPUT_FILENAME_MAX_LEN (256)

typedef struct BMtoBMP_BitmapImage_s
{
  char filename[BMtoBMP_OUTPUT_FILENAME_MAX_LEN];
  uint32_t width;
  uint32_t height;
  uint8_t **data;
} BMtoBMP_BitmapImage_t;

/**
 *  destroy_img_data - frees all memory alloc'd by a `BMtoBMP_BitmapImage_t`.
 *
 *  @param  img some `BMtoBMP_BitmapImage_t`.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  read_uint32_from_file - reads a little endian uint32 from the given file.
 *
 *  @param  fptr  file ptr (`FILE *`) where the int32 should be read from.
 *  @param  x the output.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  write_le_int32_to_file - writes a little endian (u)int32 to the given file.
 *
 *  @param  fptr  file ptr (`FILE *`) where the int32 should be written.
 *  @param  x some `uint32_t` or `int32_t`.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  write_le_int16_to_file - writes a little endian (u)int16 to the given file.
 *
 *  @param  fptr  file ptr (`FILE *`) where the int16 should be written.
 *  @param  x some `uint16_t` or `int16_t`.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  write_byte_to_file - writes a given (u)int8 to the given file.
 *
 *  @param  fptr  file ptr (`FILE *`) where the int8 should be written.
 *  @param  x some `uint8_t` or `int8_t`.
 *  @return zero on success, non-zero on failure.
 */
static int8_t
write_byte_to_file (FILE *fptr, uint8_t x)
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

/**
 *  write_string_to_file - writes a given string to the given file.
 *
 *  @param  fptr  file ptr (`FILE *`) where the str should be written.
 *  @param  s a null-terminated c string.
 *  @param  s_len `size_t` of the `s`'s length
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  output_image_to_file - writes bitmap file to output file specified by the
 *  given `BMtoBMP_BitmapImage_t`'s `filename` data field.
 *
 *  @param  img the `BMtoBMP_BitmapImage_t` to be written.
 *  @return zero on success, non-zero on failure.
 */
static int8_t
output_image_to_file (BMtoBMP_BitmapImage_t *img)
{
  FILE *output = fopen (img->filename, "wb");
  if (output == NULL)
    {
#ifdef BMtoBMP_DEBUG_OUTPUT
      fprintf (stderr, "[BMtoBMP] fopen error: could not create file, %s.\n",
               img->filename);
#endif /* BMtoBMP_DEBUG_OUTPUT */
      return -1;
    }

  /* Make sure we're at the beginning of the file. */
  fseek (output, 0x0, SEEK_SET);

  const uint32_t pixel_data_size
      = (img->width * img->height * BMtoBMP_BYTES_PER_PIXEL);
  const uint32_t output_file_size = 54 + pixel_data_size;
  const int32_t ppm_resolution = 0x0B13; // pixel per meter

  /* Populate bitmap file header. (BITMAPINFOHEADER) */
  if (write_string_to_file (output, "BM", 2) != 0 // file signature
      || write_le_int32_to_file (output, output_file_size) != 0
      || write_le_int32_to_file (output, 0x0) != 0  // Reserved
      || write_le_int32_to_file (output, 0x36) != 0 // pixel array offset
      || write_le_int32_to_file (output, 0x28) != 0 // DIB header size
      || write_le_int32_to_file (output, img->width) != 0
      || write_le_int32_to_file (output, img->height) != 0
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
      goto clean_up;
    }

  /* Output image data to file. */
  const uint32_t pad = (4 - (img->width * BMtoBMP_BYTES_PER_PIXEL) % 4) % 4;
  for (uint32_t i = 0; i < img->height; i++)
    {
      for (uint32_t j = 0; j < img->width; j++)
        {
          for (uint32_t k = 0; k < BMtoBMP_BYTES_PER_PIXEL; k++)
            {
              if (write_byte_to_file (output, img->data[i][j * 3 + k]) != 0)
                {
                  goto clean_up;
                }
            }
        }
      for (uint32_t p = 0; p < pad; p++)
        {
          write_byte_to_file (output, 0x0);
        }
    }

  /* Update file size in header. */
  size_t size = ftell (output);
  fseek (output, 0x22, SEEK_SET);
  write_le_int32_to_file (output, size - 54);

  fclose (output);
  return 0;
clean_up:
  fclose (output);
  return -1;
}

/**
 *  process_image - reads in palette indexes from `bm_file` and converts them
 *  to rgb values, storing the output in `img`'s `data` data field.
 *
 *  @param  bm_file  file ptr (`FILE *`)to some open bm_file.
 *  @param  pal_file  file ptr (`FILE *`) to some open pal_file.
 *  @param  img some `BMtoBMP_BitmapImage_t` for writing the data into.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  allocate_img_data - allocs memory for a `BMtoBMP_BitmapImage_t`'s `data`
 *  field.
 *
 *  @param  img some `BMtoBMP_BitmapImage_t`.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  create_image - uses image height and width data to create and init a
 *  `BMtoBMP_BitmapImage_t`.
 *
 *  @param  bm_file  file ptr (`FILE *`)to some open bm_file.
 *  @param  img some uninitialized `BMtoBMP_BitmapImage_t`.
 *  @return zero on success, non-zero on failure.
 */
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

/**
 *  BMtoBMP_convert_image - The primary function for converting a BM image file
 *  to BMP format.
 *
 *  @param  bm_file  file ptr (`FILE *`)to some open bm_file.
 *  @param  pal_file  file ptr (`FILE *`) to some open pal_file.
 *  @param  output_filename sz of the desired output filename, max len = 250.
 *  @return zero on success, non-zero on failure.
 */
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

  if (output_image_to_file (&img) != 0)
    goto clean_up;

  destroy_img_data (&img);
  return 0;
clean_up:
  destroy_img_data (&img);
  return -1;
}

#endif /* _BM_TO_BITMAP_CONVERTER_H_ */
