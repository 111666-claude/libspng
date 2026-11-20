/* Byte-exact tests for SPNG_FMT_G8/GA8/GA16 decoding from all color types */
#include <spng.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int n_fail = 0;

#define FAIL_STR "FAIL"
#define PASS_STR "PASS"

static void check_bytes(const char *name, const unsigned char *expected, const unsigned char *actual, size_t len)
{
    if(!memcmp(expected, actual, len))
    {
        printf("%s: %s\n", PASS_STR, name);
        return;
    }

    printf("%s: %s\n  expected:", FAIL_STR, name);
    for(size_t i = 0; i < len; i++) printf(" %u", expected[i]);
    printf("\n  actual:  ");
    for(size_t i = 0; i < len; i++) printf(" %u", actual[i]);
    printf("\n");

    n_fail++;
}

static void check_u16(const char *name, const uint16_t *expected, const unsigned char *actual, size_t count)
{
    uint16_t value;
    size_t i;

    for(i = 0; i < count; i++)
    {
        memcpy(&value, actual + i * 2, 2);
        if(value != expected[i]) break;
    }

    if(i == count)
    {
        printf("%s: %s\n", PASS_STR, name);
        return;
    }

    printf("%s: %s at index %zu, expected %u got %u\n", FAIL_STR, name, i, expected[i], value);

    n_fail++;
}

static void check_int(const char *name, int expected, int actual)
{
    if(expected == actual) printf("%s: %s\n", PASS_STR, name);
    else
    {
        printf("%s: %s, expected %d got %d\n", FAIL_STR, name, expected, actual);
        n_fail++;
    }
}

/* 2x1 8-bit RGB: (255,0,0), (16,32,64) */
static const unsigned char rgb8_png[] = {
137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,2,0,0,0,123,64,232,221,0,0,0,15,73,68,65,84,120,156,99,248,207,192,32,160,224,0,0,6,177,1,112,0,172,209,243,0,0,0,0,73,69,78,68,174,66,96,130
};

/* 1x1 16-bit RGB: (65535,0,0) */
static const unsigned char rgb16_png[] = {
137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,16,2,0,0,0,192,231,143,157,0,0,0,13,73,68,65,84,120,156,99,248,255,159,1,8,0,10,252,1,255,19,201,133,9,0,0,0,0,73,69,78,68,174,66,96,130
};

/* 2x1 8-bit palette: index 0 = (255,0,0) alpha 0, index 1 = (0,0,255) alpha 128 */
static const unsigned char pal_trns_png[] = {
137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,3,0,0,0,195,252,143,184,0,0,0,6,80,76,84,69,255,0,0,0,0,255,108,161,253,142,0,0,0,2,116,82,78,83,0,128,155,43,78,24,0,0,0,11,73,68,65,84,120,156,99,96,96,4,0,0,4,0,2,191,122,63,74,0,0,0,0,73,69,78,68,174,66,96,130
};

/* 2x1 8-bit grayscale-alpha: (10,20), (30,40) */
static const unsigned char ga8_png[] = {
137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,4,0,0,0,94,43,183,1,0,0,0,13,73,68,65,84,120,156,99,224,18,145,211,0,0,0,205,0,101,106,153,132,66,0,0,0,0,73,69,78,68,174,66,96,130
};

/* 2x1 8-bit grayscale with tRNS gray=7: 7, 200 */
static const unsigned char g8_trns_png[] = {
137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,0,0,0,0,209,73,32,86,0,0,0,2,116,82,78,83,0,7,232,247,88,155,0,0,0,11,73,68,65,84,120,156,99,96,63,1,0,0,217,0,208,215,166,34,60,0,0,0,0,73,69,78,68,174,66,96,130
};

static spng_ctx *make_ctx(const unsigned char *png, size_t png_size)
{
    spng_ctx *ctx = spng_ctx_new(0);
    if(ctx == NULL) return NULL;
    if(spng_set_png_buffer(ctx, png, png_size))
    {
        spng_ctx_free(ctx);
        return NULL;
    }
    return ctx;
}

/* Decode the whole image, returns 0 on success */
static int decode(const unsigned char *png, size_t png_size, int fmt, int flags, unsigned char *out, size_t len)
{
    spng_ctx *ctx = make_ctx(png, png_size);
    if(ctx == NULL) return -1;

    int ret = spng_decode_image(ctx, out, len, fmt, flags);

    spng_ctx_free(ctx);

    return ret;
}

static void test_rgb8(void)
{
    unsigned char out[16];

    /* (6968*255) >> 15 = 54, (6968*16 + 23434*32 + 2366*64) >> 15 = 30 */
    static const unsigned char g8[] = {54, 30};
    static const unsigned char ga8[] = {54, 255, 30, 255};
    static const uint16_t ga16[] = {54 * 257, 65535, 30 * 257, 65535};

    if(decode(rgb8_png, sizeof(rgb8_png), SPNG_FMT_G8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("rgb8 -> G8", g8, out, 2);

    if(decode(rgb8_png, sizeof(rgb8_png), SPNG_FMT_GA8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("rgb8 -> GA8", ga8, out, 4);

    if(decode(rgb8_png, sizeof(rgb8_png), SPNG_FMT_GA16, 0, out, sizeof(out)))
        n_fail++;
    else
        check_u16("rgb8 -> GA16", ga16, out, 4);
}

static void test_rgb16(void)
{
    unsigned char out[16];

    /* (6968*65535 + 16384) >> 15 = 13936, 13936 >> 8 = 54 */
    static const unsigned char g8[] = {54};
    static const unsigned char ga8[] = {54, 255};
    static const uint16_t ga16[] = {13936, 65535};

    if(decode(rgb16_png, sizeof(rgb16_png), SPNG_FMT_G8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("rgb16 -> G8", g8, out, 1);

    if(decode(rgb16_png, sizeof(rgb16_png), SPNG_FMT_GA8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("rgb16 -> GA8", ga8, out, 2);

    if(decode(rgb16_png, sizeof(rgb16_png), SPNG_FMT_GA16, 0, out, sizeof(out)))
        n_fail++;
    else
        check_u16("rgb16 -> GA16", ga16, out, 2);
}

static void test_palette_trns(void)
{
    unsigned char out[16];

    /* (2366*255) >> 15 = 18 */
    static const unsigned char g8[] = {54, 18};
    static const unsigned char ga8_opaque[] = {54, 255, 18, 255};
    static const unsigned char ga8_trns[] = {54, 0, 18, 128};
    static const uint16_t ga16_trns[] = {54 * 257, 0, 18 * 257, 128 * 257};

    if(decode(pal_trns_png, sizeof(pal_trns_png), SPNG_FMT_G8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("palette+tRNS -> G8", g8, out, 2);

    /* Without SPNG_DECODE_TRNS transparency is not applied */
    if(decode(pal_trns_png, sizeof(pal_trns_png), SPNG_FMT_GA8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("palette+tRNS -> GA8", ga8_opaque, out, 4);

    if(decode(pal_trns_png, sizeof(pal_trns_png), SPNG_FMT_GA8, SPNG_DECODE_TRNS, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("palette+tRNS -> GA8 TRNS", ga8_trns, out, 4);

    if(decode(pal_trns_png, sizeof(pal_trns_png), SPNG_FMT_GA16, SPNG_DECODE_TRNS, out, sizeof(out)))
        n_fail++;
    else
        check_u16("palette+tRNS -> GA16 TRNS", ga16_trns, out, 4);
}

static void test_ga8_source(void)
{
    unsigned char out[16];

    static const unsigned char g8[] = {10, 30};
    static const unsigned char ga8[] = {10, 20, 30, 40};
    static const uint16_t ga16[] = {10 * 257, 20 * 257, 30 * 257, 40 * 257};

    if(decode(ga8_png, sizeof(ga8_png), SPNG_FMT_G8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("ga8 -> G8", g8, out, 2);

    if(decode(ga8_png, sizeof(ga8_png), SPNG_FMT_GA8, 0, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("ga8 -> GA8", ga8, out, 4);

    if(decode(ga8_png, sizeof(ga8_png), SPNG_FMT_GA16, 0, out, sizeof(out)))
        n_fail++;
    else
        check_u16("ga8 -> GA16", ga16, out, 4);
}

static void test_g8_trns(void)
{
    unsigned char out[16];

    static const unsigned char g8[] = {7, 200};
    static const unsigned char ga8[] = {7, 0, 200, 255};
    static const uint16_t ga16[] = {7 * 257, 0, 200 * 257, 65535};

    if(decode(g8_trns_png, sizeof(g8_trns_png), SPNG_FMT_G8, SPNG_DECODE_TRNS, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("g8+tRNS -> G8", g8, out, 2);

    if(decode(g8_trns_png, sizeof(g8_trns_png), SPNG_FMT_GA8, SPNG_DECODE_TRNS, out, sizeof(out)))
        n_fail++;
    else
        check_bytes("g8+tRNS -> GA8 TRNS", ga8, out, 4);

    if(decode(g8_trns_png, sizeof(g8_trns_png), SPNG_FMT_GA16, SPNG_DECODE_TRNS, out, sizeof(out)))
        n_fail++;
    else
        check_u16("g8+tRNS -> GA16 TRNS", ga16, out, 4);
}

static void test_rows(void)
{
    unsigned char out[4] = {0};
    unsigned char row[4];
    struct spng_row_info row_info;

    /* Row-by-row decoding of rgb8_png -> GA8 */
    spng_ctx *ctx = make_ctx(rgb8_png, sizeof(rgb8_png));
    if(ctx == NULL)
    {
        n_fail++;
        return;
    }

    int ret = spng_decode_image(ctx, NULL, 0, SPNG_FMT_GA8, SPNG_DECODE_PROGRESSIVE);
    check_int("ga8 progressive init", 0, ret);

    if(!ret)
    {
        do
        {
            ret = spng_get_row_info(ctx, &row_info);
            if(ret) break;

            ret = spng_decode_row(ctx, row, sizeof(row));
            if(ret && ret != SPNG_EOI) break;

            memcpy(out + row_info.row_num * 4, row, 4);
        }while(!ret);

        check_int("ga8 row decode end", SPNG_EOI, ret);

        static const unsigned char ga8[] = {54, 255, 30, 255};
        check_bytes("rgb8 -> GA8 (rows)", ga8, out, 4);
    }

    spng_ctx_free(ctx);

    /* Undersized row buffer */
    ctx = make_ctx(rgb8_png, sizeof(rgb8_png));
    if(ctx == NULL)
    {
        n_fail++;
        return;
    }

    ret = spng_decode_image(ctx, NULL, 0, SPNG_FMT_GA8, SPNG_DECODE_PROGRESSIVE);
    if(!ret) ret = spng_decode_row(ctx, row, 1);
    check_int("row buffer too small", SPNG_EBUFSIZ, ret);

    spng_ctx_free(ctx);
}

static void test_errors(void)
{
    unsigned char out[16];
    size_t out_size = 0;

    spng_ctx *ctx = make_ctx(rgb8_png, sizeof(rgb8_png));
    if(ctx == NULL)
    {
        n_fail++;
        return;
    }

    check_int("image size G8", 0, spng_decoded_image_size(ctx, SPNG_FMT_G8, &out_size));
    check_int("image size G8 value", 2, (int)out_size);

    check_int("image size GA16", 0, spng_decoded_image_size(ctx, SPNG_FMT_GA16, &out_size));
    check_int("image size GA16 value", 8, (int)out_size);

    /* Output buffer too small */
    check_int("G8 buffer too small", SPNG_EBUFSIZ,
              spng_decode_image(ctx, out, 1, SPNG_FMT_G8, 0));
    spng_ctx_free(ctx);

    ctx = make_ctx(rgb8_png, sizeof(rgb8_png));
    if(ctx == NULL)
    {
        n_fail++;
        return;
    }

    /* Invalid format constants */
    check_int("invalid fmt 0", SPNG_EFMT, spng_decode_image(ctx, out, sizeof(out), 0, 0));
    spng_ctx_free(ctx);

    ctx = make_ctx(rgb8_png, sizeof(rgb8_png));
    if(ctx == NULL)
    {
        n_fail++;
        return;
    }

    check_int("invalid fmt combination", SPNG_EFMT,
              spng_decode_image(ctx, out, sizeof(out), SPNG_FMT_G8 | SPNG_FMT_GA8, 0));
    spng_ctx_free(ctx);
}

/* Encode the same image as Adam7 and non-interlaced, gray output must be identical */
static void test_interlaced(void)
{
    unsigned char rgba[8 * 8 * 4];
    unsigned char out_a[8 * 8 * 4], out_b[8 * 8 * 4];

    /* Fixed pattern covering all Adam7 passes */
    for(unsigned y = 0; y < 8; y++)
    {
        for(unsigned x = 0; x < 8; x++)
        {
            unsigned char *px = rgba + (y * 8 + x) * 4;
            px[0] = (unsigned char)(x * 32);
            px[1] = (unsigned char)(y * 32);
            px[2] = (unsigned char)((x + y) * 16);
            px[3] = (unsigned char)(255 - (x + y) * 8);
        }
    }

    static const int fmts[] = { SPNG_FMT_G8, SPNG_FMT_GA8, SPNG_FMT_GA16 };
    static const char *fmt_names[] = { "G8", "GA8", "GA16" };

    for(size_t f = 0; f < sizeof(fmts) / sizeof(fmts[0]); f++)
    {
        int fmt = fmts[f];
        int pixel_size = fmt == SPNG_FMT_G8 ? 1 : (fmt == SPNG_FMT_GA8 ? 2 : 4);

        for(int interlace = 0; interlace <= 1; interlace++)
        {
            spng_ctx *enc = spng_ctx_new(SPNG_CTX_ENCODER);
            struct spng_ihdr ihdr = {0};

            ihdr.width = 8;
            ihdr.height = 8;
            ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
            ihdr.bit_depth = 8;
            ihdr.interlace_method = interlace;

            spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);

            int ret = spng_set_ihdr(enc, &ihdr);
            if(ret)
            {
                printf("interlaced %s: spng_set_ihdr() error: %s\n", fmt_names[f], spng_strerror(ret));
                n_fail++;
                spng_ctx_free(enc);
                return;
            }

            ret = spng_encode_image(enc, rgba, sizeof(rgba), SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
            if(ret)
            {
                printf("interlaced %s: spng_encode_image() error: %s\n", fmt_names[f], spng_strerror(ret));
                n_fail++;
                spng_ctx_free(enc);
                return;
            }

            size_t png_size = 0;
            int error = 0;
            void *png_buf = spng_get_png_buffer(enc, &png_size, &error);
            if(png_buf == NULL)
            {
                printf("interlaced %s: spng_get_png_buffer() error: %s\n", fmt_names[f], spng_strerror(error));
                n_fail++;
                spng_ctx_free(enc);
                return;
            }

            unsigned char *out = interlace ? out_b : out_a;
            memset(out, 0, sizeof(out_a));

            ret = decode(png_buf, png_size, fmt, 0, out, sizeof(out_a));
            if(ret)
            {
                printf("interlaced %s: decode error: %s\n", fmt_names[f], spng_strerror(ret));
                n_fail++;
            }

            free(png_buf);
            spng_ctx_free(enc);
        }

        char name[64];
        snprintf(name, sizeof(name), "interlaced rgba8 -> %s", fmt_names[f]);
        check_bytes(name, out_a, out_b, (size_t)8 * 8 * pixel_size);
    }
}

int main(void)
{
    test_rgb8();
    test_rgb16();
    test_palette_trns();
    test_ga8_source();
    test_g8_trns();
    test_rows();
    test_errors();
    test_interlaced();

    if(n_fail)
    {
        printf("%d test(s) failed\n", n_fail);
        return 1;
    }

    printf("all tests passed\n");
    return 0;
}
