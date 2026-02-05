#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "CoDec.h"

int main(int argc, char **argv) {

    if (argc < 2 || strcmp(argv[1], "-h") == 0) {
        print_help(argv[0]);
        return (argc < 2);
    }

    if (argc < 4) {
        fprintf(stderr, "Error: Missing arguments\n");
        print_help(argv[0]);
        return 1;
    }

    Options opts = {0};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) opts.verbose = 1;
        else if (strcmp(argv[i], "-t") == 0) opts.timing = 1;
    }

    if (opts.timing) opts.start_time = clock();

    int result = 0;

    if (strcmp(argv[1], "-c") == 0) {
        verbose_printf(&opts, "=== ENCODING MODE ===\n");

        const char *input = argv[2];
        const char *output = argv[3];
        char *pnm_tmp = NULL;

        if (!is_pnm_file(input)) {
            verbose_printf(&opts, "Input is not PNM, converting...\n");

            pnm_tmp = change_extension(input, ".pnm");
            
            if (!pnm_tmp || !convert_to_pnm(input, pnm_tmp)) {
                fprintf(stderr, "Error: PNM conversion failed\n");
                if (pnm_tmp) free(pnm_tmp);
                return 1;
            }

            input = pnm_tmp;
            verbose_printf(&opts, "Converted to %s\n", input);
        }

        if (opts.verbose) {
            long size_in = file_size(argv[2]);
            printf("Input file: %s (%ld bytes)\n", argv[2], size_in);
            printf("Output file: %s\n", output);
        }

        result = pnmtodif(input, output);

        if (result == 0 && opts.verbose) {
            long size_out = file_size(output);
            printf("Encoding successful. Final size: %ld bytes\n", size_out);
        }

        if (pnm_tmp) {
            remove(pnm_tmp);
            free(pnm_tmp);
        }
    }
    else if (strcmp(argv[1], "-d") == 0) {
        verbose_printf(&opts, "=== DECODING MODE ===\n");

        int open_image = 0;
        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) open_image = 1;
        }

        verbose_printf(&opts, "Input file: %s\n", argv[2]);
        verbose_printf(&opts, "Output file: %s\n", argv[3]);

        result = diftopnm(argv[2], argv[3]);

        if (result == 0) {
            verbose_printf(&opts, "Decoding successful.\n");
            if (open_image) {
                display_file(argv[3], "xdg-open");
            }
        }
    }
    else {
        fprintf(stderr, "Error: Unknown mode %s\n", argv[1]);
        print_help(argv[0]);
        return 1;
    }

    if (opts.timing) {
        clock_t end = clock();
        double elapsed = (double)(end - opts.start_time) / CLOCKS_PER_SEC;
        printf("\n=== EXECUTION TIME ===\n");
        printf("Execution time: %.3f seconds\n", elapsed);
    }

    return result;
}