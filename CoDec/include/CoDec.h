#ifndef CODEC_H
#define CODEC_H

#include <stdio.h>
#include <time.h>

/* ========================================================================
 * DÉFINITIONS ET STRUCTURES
 * ======================================================================== */

/** @brief Unsigned char type alias for convenience */
typedef unsigned char uchar;

/** @brief Magic number for grayscale DIF files */
#define MAGIC_GRAY 0xD1FF
/** @brief Magic number for RGB DIF files */
#define MAGIC_RGB  0xD3FF
/** @brief Number of quantization levels */
#define NUM_LEVELS 4     

/**
 * @brief Structure representing a picture/image
 * @var Picture::w Width of the image in pixels
 * @var Picture::h Height of the image in pixels
 * @var Picture::channels Number of color channels (1 for grayscale, 3 for RGB)
 * @var Picture::pixels Pointer to pixel data (row-major order)
 */
typedef struct {
    int w, h;      
    int channels;  
    uchar *pixels;
} Picture;

/**
 * @brief Structure for bitwise stream reading/writing
 * @var Stream::ptr Current pointer in the buffer
 * @var Stream::end End of the buffer
 * @var Stream::bitpos Current bit position (0-7)
 */
typedef struct {
    uchar *ptr;    
    uchar *end;    
    int bitpos;    
} Stream;

/**
 * @brief Quantizer configuration for encoding/decoding
 * @var Quantizer::levels Number of quantization levels
 * @var Quantizer::bits Number of bits for each level
 * @var Quantizer::bounds Lower bound values for each level
 */
typedef struct {
    int levels;
    int bits[NUM_LEVELS];
    int bounds[NUM_LEVELS];
} Quantizer;

/**
 * @brief Command-line options structure
 * @var Options::verbose Enable verbose output
 * @var Options::timing Enable timing measurements
 * @var Options::start_time Starting time for timing
 */
typedef struct {
    int verbose;
    int timing;
    clock_t start_time;
} Options;

/* ========================================================================
 * PROTOTYPES DES FONCTIONS (Exportées pour le main)
 * ======================================================================== */

/**
 * @brief Converts a PNM image to DIF format
 * @param input Path to input PNM file
 * @param output Path to output DIF file
 * @return 0 on success, 1 on failure
 */
int pnmtodif(const char *input, const char *output);

/**
 * @brief Converts a DIF image to PNM format
 * @param input Path to input DIF file
 * @param output Path to output PNM file
 * @return 0 on success, 1 on failure
 */
int diftopnm(const char *input, const char *output);

/**
 * @brief Checks if a tool is available in the system
 * @param tool Name of the tool to check
 * @return 1 if available, 0 otherwise
 */
int check_tool(const char *tool);

/**
 * @brief Displays a file using a specified viewer
 * @param path Path to the file to display
 * @param viewer Name or path of the viewer program
 */
void display_file(const char *path, const char *viewer);

/**
 * @brief Checks if a file is in DIF format by reading its magic number
 * @param path Path to the file
 * @return 1 if DIF file, 0 otherwise
 */
int is_dif_file(const char *path);

/**
 * @brief Checks if a file is in PNM format (P5 or P6 magic number)
 * @param path Path to the file
 * @return 1 if PNM file, 0 otherwise
 */
int is_pnm_file(const char *path);

/**
 * @brief Changes the file extension of a path
 * @param path Original file path
 * @param ext New extension (including the dot, e.g. ".dif")
 * @return Newly allocated string with changed extension, or NULL on error
 */
char *change_extension(const char *path, const char *ext);

/**
 * @brief Gets the file size in bytes
 * @param path Path to the file
 * @return File size in bytes, or -1 if file cannot be opened
 */
long file_size(const char *path);

/**
 * @brief Calculates the raw (uncompressed) size of a PNM image
 * @param pnm Path to the PNM file
 * @return Size in bytes (width * height * channels), or -1 on error
 */
long get_raw_size(const char *pnm);

/**
 * @brief Converts an image to PNM format using ImageMagick
 * @param input Path to input image file
 * @param output Path to output PNM file
 * @return 1 on success, 0 on failure
 */
int convert_to_pnm(const char *input, const char *output);

/**
 * @brief Prints usage information for the program
 * @param prog Program name (typically argv[0])
 */
void print_usage(const char *prog);

/**
 * @brief Prints help information for the program
 * @param prog Program name (typically argv[0])
 */
void print_help(const char *prog);

/**
 * @brief Prints a message only if verbose mode is enabled
 * @param opts Pointer to Options structure
 * @param format Printf-style format string
 * @param ... Variable arguments for printf
 */
void verbose_printf(Options *opts, const char *format, ...);

#endif