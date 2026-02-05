#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#include "CoDec.h" // On inclut le header qui contient les structures Picture, etc.


/**
 * @brief Initializes a stream for writing bits
 * @param s Pointer to the Stream structure to initialize
 * @param buf Buffer to write bits into
 * @param size Size of the buffer in bytes
 */
static void stream_init_write(Stream *s, uchar *buf, int size) {
    s->ptr = buf;
    s->end = buf + size;
    s->bitpos = 0;
    if (size > 0) *s->ptr = 0;
}

/**
 * @brief Initializes a stream for reading bits
 * @param s Pointer to the Stream structure to initialize
 * @param buf Buffer to read bits from
 * @param size Size of the buffer in bytes
 */
static void stream_init_read(Stream *s, uchar *buf, int size) {
    s->ptr = buf;
    s->end = buf + size;
    s->bitpos = 0;
}

/**
 * @brief Writes bits to a stream
 * @param s Pointer to the Stream structure
 * @param value Value containing the bits to write
 * @param nbits Number of bits to write from the value
 * @return 1 on success, 0 on failure (buffer overflow)
 */
static int stream_write_bits(Stream *s, unsigned int value, int nbits) {
    for (int i = nbits - 1; i >= 0; i--) {
        if (s->ptr >= s->end) return 0;
        int bit = (value >> i) & 1;
        *s->ptr |= (bit << (7 - s->bitpos));
        s->bitpos++;
        if (s->bitpos == 8) {
            s->ptr++;
            s->bitpos = 0;
            if (s->ptr < s->end) *s->ptr = 0;
        }
    }
    return 1;
}

/**
 * @brief Reads bits from a stream
 * @param s Pointer to the Stream structure
 * @param nbits Number of bits to read
 * @param value Pointer to store the read value
 * @return 1 on success, 0 on failure (end of buffer reached)
 */
static int stream_read_bits(Stream *s, int nbits, unsigned int *value) {
    unsigned int result = 0;
    for (int i = 0; i < nbits; i++) {
        if (s->ptr >= s->end) return 0;
        int bit = (*s->ptr >> (7 - s->bitpos)) & 1;
        result = (result << 1) | bit;
        s->bitpos++;
        if (s->bitpos == 8) {
            s->ptr++;
            s->bitpos = 0;
        }
    }
    *value = result;
    return 1;
}

/**
 * @brief Calculates the number of bytes used in a stream
 * @param s Pointer to the Stream structure
 * @param buf Original buffer pointer
 * @return Number of bytes used (rounded up from bits)
 */
static int stream_bytes_used(Stream *s, uchar *buf) {
    int bits = (s->ptr - buf) * 8 + s->bitpos;
    return (bits + 7) / 8;
}

/* ========================================================================
 * GESTION DES IMAGES PNM (picture_load/save restent static si non dans .h)
 * ======================================================================== */

/**
 * @brief Skips whitespace and comments in PNM file
 * @param fp File pointer
 */
static void skip_whitespace_comments(FILE *fp) {
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (isspace(ch)) continue;
        if (ch == '#') {
            while ((ch = fgetc(fp)) != '\n' && ch != EOF);
        } else {
            ungetc(ch, fp);
            break;
        }
    }
}

/**
 * @brief Loads a PNM image from file
 * @param path Path to the PNM file
 * @param pic Pointer to Picture structure to fill
 * @return 1 on success, 0 on failure
 */
static int picture_load(const char *path, Picture *pic) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    char magic[3] = {0};
    if (fscanf(fp, "%2s", magic) != 1) { fclose(fp); return 0; }
    if (strcmp(magic, "P5") == 0) pic->channels = 1;
    else if (strcmp(magic, "P6") == 0) pic->channels = 3;
    else { fclose(fp); return 0; }
    skip_whitespace_comments(fp);
    if (fscanf(fp, "%d %d", &pic->w, &pic->h) != 2) { fclose(fp); return 0; }
    int maxval;
    skip_whitespace_comments(fp);
    if (fscanf(fp, "%d", &maxval) != 1 || maxval != 255) { fclose(fp); return 0; }
    fgetc(fp); 
    int total = pic->w * pic->h * pic->channels;
    pic->pixels = malloc(total);
    if (!pic->pixels) { fclose(fp); return 0; }
    if (fread(pic->pixels, 1, total, fp) != (size_t)total) { free(pic->pixels); fclose(fp); return 0; }
    fclose(fp);
    return 1;
}

/**
 * @brief Saves a PNM image to file
 * @param path Path to save the PNM file
 * @param pic Pointer to Picture structure
 * @return 1 on success, 0 on failure
 */
static int picture_save(const char *path, Picture *pic) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    const char *magic = (pic->channels == 3) ? "P6" : "P5";
    fprintf(fp, "%s\n%d %d\n255\n", magic, pic->w, pic->h);
    int total = pic->w * pic->h * pic->channels;
    if (fwrite(pic->pixels, 1, total, fp) != (size_t)total) { fclose(fp); return 0; }
    fclose(fp);
    return 1;
}

/**
 * @brief Frees memory allocated for a picture
 * @param pic Pointer to Picture structure
 */
static void picture_free(Picture *pic) {
    if (pic && pic->pixels) { free(pic->pixels); pic->pixels = NULL; }
}

/* ========================================================================
 * QUANTIFICATION ET ENCODAGE
 * ======================================================================== */

/**
 * @brief Maps a byte value to a quantization level
 * @param val Value to map (0-255)
 * @return Quantization level (0-3)
 */
static int map_value(uchar val) {
    if (val >= 22) return 3;
    if (val >= 6)  return 2;
    if (val >= 2)  return 1;
    return 0;
}

/**
 * @brief Gets the number of bits needed to encode a quantization level
 * @param level Quantization level (0-3)
 * @return Number of bits
 */
static int level_bits(int level) {
    const int bits_table[NUM_LEVELS] = {1, 2, 4, 8};
    return bits_table[level];
}

/**
 * @brief Gets the length of the prefix code for a quantization level
 * @param level Quantization level (0-3)
 * @return Prefix length in bits
 */
static int prefix_length(int level) {
    const int len_table[NUM_LEVELS] = {1, 2, 3, 3};
    return len_table[level];
}

/**
 * @brief Gets the prefix code for a quantization level
 * @param level Quantization level (0-3)
 * @return Prefix code as unsigned int
 */
static unsigned int prefix_code(int level) {
    const unsigned int code_table[NUM_LEVELS] = {0b0, 0b10, 0b110, 0b111};
    return code_table[level];
}

/**
 * @brief Gets the lower bound value for a quantization level
 * @param level Quantization level (0-3)
 * @return Lower bound value
 */
static int level_bound(int level) {
    const int bound_table[NUM_LEVELS] = {0, 2, 6, 22};
    return bound_table[level];
}

/**
 * @brief Encodes a signed difference using zigzag encoding
 * @param diff Signed difference value
 * @return Zigzag encoded value
 */
static uchar zigzag_encode(int diff) {
    return (diff >= 0) ? (2 * diff) : (-2 * diff - 1);
}

/**
 * @brief Decodes a zigzag encoded value to signed difference
 * @param encoded Zigzag encoded value
 * @return Signed difference
 */
static int zigzag_decode(uchar encoded) {
    return (encoded & 1) ? -(encoded + 1) / 2 : encoded / 2;
}

/**
 * @brief Encodes a value to stream with quantization prefix and data
 * @param s Pointer to the Stream structure
 * @param val Value to encode
 * @return 1 on success, 0 on failure
 */
static int encode_value(Stream *s, uchar val) {
    int level = map_value(val);
    if (!stream_write_bits(s, prefix_code(level), prefix_length(level))) return 0;
    unsigned int data = val - level_bound(level);
    if (!stream_write_bits(s, data, level_bits(level))) return 0;
    return 1;
}

/**
 * @brief Decodes a value from stream with quantization
 * @param s Pointer to the Stream structure
 * @param val Pointer to store the decoded value
 * @param q Pointer to Quantizer configuration
 * @return 1 on success, 0 on failure
 */
static int decode_value(Stream *s, uchar *val, Quantizer *q) {
    unsigned int bit;
    int level;
    if (!stream_read_bits(s, 1, &bit)) return 0;
    if (bit == 0) level = 0;
    else {
        if (!stream_read_bits(s, 1, &bit)) return 0;
        if (bit == 0) level = 1;
        else {
            if (!stream_read_bits(s, 1, &bit)) return 0;
            level = (bit == 0) ? 2 : 3;
        }
    }
    unsigned int data;
    if (!stream_read_bits(s, q->bits[level], &data)) return 0;
    *val = data + q->bounds[level];
    return 1;
}

/* ========================================================================
 * EN-TÊTE DIF ET FONCTIONS PUBLIQUES
 * ======================================================================== */

/**
 * @brief Writes a 16-bit unsigned integer to file in little-endian format
 * @param fp File pointer
 * @param val Value to write
 * @return 1 on success, 0 on failure
 */
static int write_u16(FILE *fp, unsigned short val) {
    uchar bytes[2] = {val & 0xFF, (val >> 8) & 0xFF};
    return fwrite(bytes, 1, 2, fp) == 2;
}

/**
 * @brief Reads a 16-bit unsigned integer from file in little-endian format
 * @param fp File pointer
 * @param val Pointer to store the read value
 * @return 1 on success, 0 on failure
 */
static int read_u16(FILE *fp, unsigned short *val) {
    uchar bytes[2];
    if (fread(bytes, 1, 2, fp) != 2) return 0;
    *val = bytes[0] | (bytes[1] << 8);
    return 1;
}


/**
 * @brief Converts a PNM image to DIF format
 * @param input Path to input PNM file
 * @param output Path to output DIF file
 * @return 0 on success, 1 on failure
 */
int pnmtodif(const char *input, const char *output) {
    Picture pic;
    if (!picture_load(input, &pic)) return 1;
    int total = pic.w * pic.h * pic.channels;
    uchar *reduced = malloc(total);
    for (int i = 0; i < total; i++) reduced[i] = pic.pixels[i] >> 1;
    
    int bufsize = (total * 11 + 7) / 8 + 16;
    uchar *buffer = calloc(bufsize, 1);
    Stream stream;
    stream_init_write(&stream, buffer, bufsize);

    int prev[3] = {0};
    for (int c = 0; c < pic.channels; c++) prev[c] = reduced[c];

    for (int i = 1; i < pic.w * pic.h; i++) {
        for (int c = 0; c < pic.channels; c++) {
            int current = reduced[i * pic.channels + c];
            uchar encoded = zigzag_encode(current - prev[c]);
            prev[c] = current;
            if (!encode_value(&stream, encoded)) { /* gestion erreur */ }
        }
    }

    FILE *out = fopen(output, "wb");
    write_u16(out, (pic.channels == 3) ? MAGIC_RGB : MAGIC_GRAY);
    write_u16(out, pic.w);
    write_u16(out, pic.h);
    uchar nl = NUM_LEVELS; fwrite(&nl, 1, 1, out);
    for(int i=0; i<NUM_LEVELS; i++) { uchar b = level_bits(i); fwrite(&b, 1, 1, out); }
    
    fwrite(reduced, 1, pic.channels, out);
    fwrite(buffer, 1, stream_bytes_used(&stream, buffer), out);
    
    fclose(out); free(buffer); free(reduced); picture_free(&pic);
    return 0;
}

/**
 * @brief Converts a DIF image to PNM format
 * @param input Path to input DIF file
 * @param output Path to output PNM file
 * @return 0 on success, 1 on failure
 */
int diftopnm(const char *input, const char *output) {
    FILE *in = fopen(input, "rb");
    if (!in) return 1;
    int w, h, chans; Quantizer quant;
    unsigned short magic, w16, h16;
    read_u16(in, &magic); chans = (magic == MAGIC_RGB) ? 3 : 1;
    read_u16(in, &w16); read_u16(in, &h16); w = w16; h = h16;
    uchar nl; fread(&nl, 1, 1, in);
    quant.levels = nl;
    quant.bounds[0] = 0;
    for(int i=0; i<nl; i++) { 
        uchar b; fread(&b, 1, 1, in); quant.bits[i] = b; 
        if(i>0) quant.bounds[i] = quant.bounds[i-1] + (1 << quant.bits[i-1]);
    }
    
    int total = w * h * chans;
    uchar first[3]; fread(first, 1, chans, in);
    
    long pos = ftell(in); fseek(in, 0, SEEK_END);
    long csize = ftell(in) - pos; fseek(in, pos, SEEK_SET);
    uchar *comp = malloc(csize); fread(comp, 1, csize, in);
    fclose(in);

    Stream s; stream_init_read(&s, comp, csize);
    uchar *red = malloc(total);
    int prev[3];
    for(int c=0; c<chans; c++) { red[c] = first[c]; prev[c] = first[c]; }

    for (int i = 1; i < w * h; i++) {
        for (int c = 0; c < chans; c++) {
            uchar enc; decode_value(&s, &enc, &quant);
            int current = prev[c] + zigzag_decode(enc);
            prev[c] = current;
            red[i * chans + c] = current;
        }
    }
    
    Picture pic = {w, h, chans, malloc(total)};
    for(int i=0; i<total; i++) pic.pixels[i] = red[i] << 1;
    picture_save(output, &pic);
    
    free(comp); free(red); free(pic.pixels);
    return 0;
}

/* ========================================================================
 * UTILITAIRES PUBLICS (SANS STATIC)
 * ======================================================================== */

/**
 * @brief Checks if a tool is available in the system
 * @param tool Name of the tool to check
 * @return 1 if available, 0 otherwise
 */
int check_tool(const char *tool) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s > /dev/null 2>&1", tool);
    return system(cmd) == 0;
}

/**
 * @brief Displays a file using a specified viewer
 * @param path Path to the file to display
 * @param viewer Name or path of the viewer program
 */
void display_file(const char *path, const char *viewer) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s \"%s\" &", viewer, path);
    if (system(cmd) != 0) {
        // Fallback to xdg-open if the viewer command fails
        snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", path);
        system(cmd);
    }
}

/**
 * @brief Checks if a file is in DIF format by reading its magic number
 * @param path Path to the file
 * @return 1 if DIF file, 0 otherwise
 */
int is_dif_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    uchar bytes[2];
    int res = (fread(bytes, 1, 2, fp) == 2);
    fclose(fp);
    if (!res) return 0;
    unsigned short magic = bytes[0] | (bytes[1] << 8);
    return (magic == MAGIC_GRAY || magic == MAGIC_RGB);
}

/**
 * @brief Checks if a file is in PNM format (P5 or P6 magic number)
 * @param path Path to the file
 * @return 1 if PNM file, 0 otherwise
 */
int is_pnm_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    char magic[3] = {0};
    int res = (fscanf(fp, "%2s", magic) == 1);
    fclose(fp);
    return res && (strcmp(magic, "P5") == 0 || strcmp(magic, "P6") == 0);
}

/**
 * @brief Changes the file extension of a path
 * @param path Original file path
 * @param ext New extension (including the dot, e.g. ".dif")
 * @return Newly allocated string with changed extension, or NULL on error
 */
char *change_extension(const char *path, const char *ext) {
    const char *dot = strrchr(path, '.');
    size_t base_len = dot ? (size_t)(dot - path) : strlen(path);
    char *result = malloc(base_len + strlen(ext) + 1);
    if (!result) return NULL;
    memcpy(result, path, base_len);
    result[base_len] = '\0';
    strcat(result, ext);
    return result;
}

/**
 * @brief Gets the file size in bytes
 * @param path Path to the file
 * @return File size in bytes, or -1 if file cannot be opened
 */
long file_size(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

/**
 * @brief Calculates the raw (uncompressed) size of a PNM image
 * @param pnm Path to the PNM file
 * @return Size in bytes (width * height * channels), or -1 on error
 */
long get_raw_size(const char *pnm) {
    Picture pic;
    if (!picture_load(pnm, &pic)) return -1;
    long size = pic.w * pic.h * pic.channels;
    picture_free(&pic);
    return size;
}

/**
 * @brief Converts an image to PNM format using ImageMagick
 * @param input Path to input image file
 * @param output Path to output PNM file
 * @return 1 on success, 0 on failure
 */
int convert_to_pnm(const char *input, const char *output) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
                 "magick \"%s\" -depth 8 -define pnm:format=binary \"%s\"",
                 input, output);
    snprintf(cmd, sizeof(cmd), "convert \"%s\" \"%s\"", input, output);
    return system(cmd) == 0;
}

/**
 * @brief Prints usage information for the program
 * @param prog Program name (typically argv[0])
 */
void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] fichier...\n\n"
                    "Options: -h (aide), -v (verbeux), -t (temps), -V <prog> (viewer)\n", prog);
}

/**
 * @brief Prints help information for the program
 * @param prog Program name (typically argv[0])
 */
void print_help(const char *prog) {
    printf("Usage: %s <mode> <input> <output> [options]\n\n", prog);
    printf("Modes:\n");
    printf("  -c              Encode mode (PNM to DIF)\n");
    printf("  -d              Decode mode (DIF to PNM)\n\n");
    printf("Arguments:\n");
    printf("  <input>         Input file path\n");
    printf("  <output>        Output file path\n\n");
    printf("Options:\n");
    printf("  -v              Enable verbose output\n");
    printf("  -t              Enable timing measurements\n");
    printf("  -o              Open image with viewer (decode mode only)\n");
    printf("  -h              Display this help message\n\n");
    printf("Examples:\n");
    printf("  %s -c image.pnm image.dif -v\n", prog);
    printf("  %s -d image.dif image.pnm -t -o\n", prog);
}

/**
 * @brief Prints a message only if verbose mode is enabled
 * @param opts Pointer to Options structure
 * @param format Printf-style format string
 * @param ... Variable arguments for printf
 */
void verbose_printf(Options *opts, const char *format, ...) {
    if (!opts || !opts->verbose) return;
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}