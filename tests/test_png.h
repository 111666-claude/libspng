#ifndef TEST_PNG_H
#define TEST_PNG_H

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__BIG_ENDIAN__)
    #define SPNGT_BIG_ENDIAN
#else
    #define SPNGT_LITTLE_ENDIAN
#endif

#include <png.h>

#include "spngt_common.h"

#include <stdlib.h>
#include <string.h>

struct buf_state
{
    unsigned char *data;
    size_t bytes_left;
};

static struct buf_state state;

void libpng_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct buf_state *state = png_get_io_ptr(png_ptr);

#if defined(SPNGT_STREAM_READ_INFO)
    printf("libpng bytes read: %lu\n", length);
#endif

    if(length > state->bytes_left)
    {
        png_error(png_ptr, "read_fn error");
    }

    memcpy(data, state->data, length);
    state->bytes_left -= length;
    state->data += length;
}

png_structp init_libpng(spngt_test_case *test_case, png_infop *iptr)
{
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL)
    {
        printf("libpng init failed\n");
        return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL)
    {
        printf("png_create_info_struct failed\n");
        return NULL;
    }

    if(test_case->source.type == SPNGT_SRC_FILE) png_init_io(png_ptr, test_case->source.file);
    else if(test_case->source.type == SPNGT_SRC_BUFFER)
    {
        state.data = test_case->source.buffer;
        state.bytes_left = test_case->source.png_size;
        png_set_read_fn(png_ptr, &state, libpng_read_fn);
    }

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        printf("libpng error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_ALWAYS, NULL, 0);

    png_read_info(png_ptr, info_ptr);

    *iptr = info_ptr;

    return png_ptr;
}

unsigned char *getimage_libpng(png_structp png_ptr, png_infop info_ptr, size_t *out_size, int fmt, int flags)
{
    unsigned char *volatile image = NULL;
    png_bytep *volatile row_pointers = NULL;

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        printf("libpng error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        if(image != NULL) free(image);
        if(row_pointers != NULL) free(row_pointers);
        return NULL;
    }

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type;
    int filter_method;
    int expand_to_16 = 0;
    int gray_from_rgb = 0;

    if(!png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                     &color_type, &interlace_type, &compression_type, &filter_method))
    {
        printf("png_get_IHDR failed\n");
        return NULL;
    }

    if(flags & SPNG_DECODE_GAMMA)
    {
        double gamma;
        if(png_get_gAMA(png_ptr, info_ptr, &gamma))
            png_set_gamma(png_ptr, 2.2, gamma);
    }

    /* libpng's png_set_rgb_to_gray() converts in linear space when the image
       has a gAMA chunk, decode to RGB(A) and convert to grayscale in the
       harness instead */
    double file_gamma;
    int gray_gamma_quirk = png_get_gAMA(png_ptr, info_ptr, &file_gamma) &&
                           (color_type & PNG_COLOR_MASK_COLOR) != 0 &&
                           (fmt & (SPNG_FMT_G8 | SPNG_FMT_GA8 | SPNG_FMT_GA16));

    if(fmt == SPNG_FMT_RGBA16)
    {
        png_set_gray_to_rgb(png_ptr);

        png_set_filler(png_ptr, 0xFFFF, PNG_FILLER_AFTER);

        /* png_set_palette_to_rgb() + png_set_tRNS_to_alpha() */
        png_set_expand_16(png_ptr);
    }
    else if(fmt == SPNG_FMT_RGBA8)
    {
        png_set_gray_to_rgb(png_ptr);

        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

        /* png_set_palette_to_rgb() + png_set_expand_gray_1_2_4_to_8() + png_set_tRNS_to_alpha() */
        png_set_expand(png_ptr);

        png_set_strip_16(png_ptr);
    }
    else if(fmt == SPNG_FMT_RGB8)
    {
        png_set_gray_to_rgb(png_ptr);

        png_set_strip_alpha(png_ptr);

        png_set_strip_16(png_ptr);
    }
    else if(fmt == SPNG_FMT_G8)
    {
        if(gray_gamma_quirk) gray_from_rgb = 1;
        else
        {
            png_set_rgb_to_gray(png_ptr, PNG_ERROR_ACTION_NONE, -1, -1);

            png_set_expand_gray_1_2_4_to_8(png_ptr);

            png_set_strip_16(png_ptr);

            png_set_strip_alpha(png_ptr);
        }
    }
    else if(fmt == SPNG_FMT_GA16)
    {
        if(gray_gamma_quirk) gray_from_rgb = 1;
        else
        {
            if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) && (flags & SPNG_DECODE_TRNS))
            {
                png_set_tRNS_to_alpha(png_ptr);
            }

            png_set_rgb_to_gray(png_ptr, PNG_ERROR_ACTION_NONE, -1, -1);

            png_set_expand_gray_1_2_4_to_8(png_ptr);

            png_set_filler(png_ptr, bit_depth == 16 ? 0xFFFF : 0xFF, PNG_FILLER_AFTER);

            expand_to_16 = bit_depth < 16;
        }
    }
    else if(fmt == SPNG_FMT_GA8)
    {
        if(gray_gamma_quirk) gray_from_rgb = 1;
        else
        {
            if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) && (flags & SPNG_DECODE_TRNS))
            {
                png_set_tRNS_to_alpha(png_ptr);
            }

            png_set_rgb_to_gray(png_ptr, PNG_ERROR_ACTION_NONE, -1, -1);

            png_set_expand_gray_1_2_4_to_8(png_ptr);

            png_set_strip_16(png_ptr);

            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }
    }
    else if(fmt == SPNGT_FMT_VIPS)
    {
        if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);

        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);

        if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

#if defined(SPNGT_LITTLE_ENDIAN)
        png_set_swap(png_ptr);
#endif
    }

    if(gray_from_rgb)
    {/* Decode to RGBA8/RGBA16, converted to grayscale after png_read_image() */
        if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);

        if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) && (flags & SPNG_DECODE_TRNS))
            png_set_tRNS_to_alpha(png_ptr);

        png_set_gray_to_rgb(png_ptr);

        png_set_filler(png_ptr, bit_depth == 16 ? 0xFFFF : 0xFF, PNG_FILLER_AFTER);
    }

#if defined(SPNGT_LITTLE_ENDIAN) /* we want host-endian values unless it's SPNG_FMT_RAW */
    if(fmt != SPNG_FMT_RAW) png_set_swap(png_ptr);
#endif

    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    size_t image_size = height * rowbytes;
    memcpy(out_size, &image_size, sizeof(size_t));

    /* Neither library does zero-padding for <8-bit images,
       but we want the images to be bit-identical for memcmp() */
    image = calloc(1, image_size);
    if(image == NULL)
    {
        printf("libpng: malloc() failed\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    row_pointers = malloc(height * sizeof(png_bytep));
    if(row_pointers == NULL)
    {
        printf("libpng: malloc() failed\n");
        free(image);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    uint32_t k;
    for(k=0; k < height; k++)
    {
        row_pointers[k] = image + k * rowbytes;
    }

    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, info_ptr);

    free(row_pointers);

    if(gray_from_rgb)
    {/* RGBA8/RGBA16 -> G8/GA8/GA16, the coefficients and rounding
        match spng's rgb_to_gray()/rgb_to_gray16() */
        size_t pixel_count = (size_t)width * height;
        size_t pixel_size = fmt == SPNG_FMT_G8 ? 1 : (fmt == SPNG_FMT_GA8 ? 2 : 4);
        size_t new_size = pixel_count * pixel_size;
        int src16 = bit_depth == 16;

        unsigned char *new_image = malloc(new_size);
        if(new_image == NULL)
        {
            printf("libpng: malloc() failed\n");
            free(image);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            return NULL;
        }

        size_t i, j = 0;
        for(i=0; i < pixel_count; i++)
        {
            uint32_t gray, alpha;

            if(src16)
            {
                uint16_t r, g, b, a;
                memcpy(&r, image + i * 8, 2);
                memcpy(&g, image + i * 8 + 2, 2);
                memcpy(&b, image + i * 8 + 4, 2);
                memcpy(&a, image + i * 8 + 6, 2);

                gray = ((uint32_t)r * 6968 + (uint32_t)g * 23434 + (uint32_t)b * 2366 + 16384) >> 15;
                alpha = a;
            }
            else
            {
                uint32_t r = image[i * 4], g = image[i * 4 + 1], b = image[i * 4 + 2];

                gray = (r * 6968 + g * 23434 + b * 2366) >> 15;
                alpha = image[i * 4 + 3];
            }

            if(fmt == SPNG_FMT_G8)
            {
                if(src16) gray >>= 8;
                new_image[j] = (unsigned char)gray;
                j += 1;
            }
            else if(fmt == SPNG_FMT_GA8)
            {
                if(src16) { gray >>= 8; alpha >>= 8; }
                new_image[j]     = (unsigned char)gray;
                new_image[j + 1] = (unsigned char)alpha;
                j += 2;
            }
            else /* SPNG_FMT_GA16 */
            {
                uint16_t g16, a16;

                if(!src16) { gray *= 257; alpha *= 257; }

                g16 = (uint16_t)gray;
                a16 = (uint16_t)alpha;

                memcpy(new_image + j, &g16, 2);
                memcpy(new_image + j + 2, &a16, 2);
                j += 4;
            }
        }

        free(image);
        image = new_image;
        memcpy(out_size, &new_size, sizeof(size_t));
    }

    if(expand_to_16)
    {/* 8 to 16-bit expansion for SPNG_FMT_GA16, png_set_expand_16() is not
        used because it also applies tRNS unconditionally */
        size_t pixel_count = (size_t)width * height;
        size_t new_size = pixel_count * 4;

        unsigned char *new_image = malloc(new_size);
        if(new_image == NULL)
        {
            printf("libpng: malloc() failed\n");
            free(image);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            return NULL;
        }

        size_t i;
        for(i=0; i < pixel_count; i++)
        {
            uint8_t gray = image[i * 2];
            uint8_t alpha = image[i * 2 + 1];

            new_image[i * 4]     = gray;
            new_image[i * 4 + 1] = gray;
            new_image[i * 4 + 2] = alpha;
            new_image[i * 4 + 3] = alpha;
        }

        free(image);
        image = new_image;
        memcpy(out_size, &new_size, sizeof(size_t));
    }

    return image;
}

#endif /* TEST_PNG_H */
