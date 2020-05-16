/* 
  FILE...: ldpc_enc.c
  AUTHOR.: Bill Cowley, David Rowe
  CREATED: Sep 2016

  RA LDPC encoder program. Using the elegant back substitution of RA
  LDPC codes.

  building: gcc ldpc_enc.c -o ldpc_enc -Wall -g
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "mpdecode_core.h"
#include "ofdm_internal.h"

/* generated by ldpc_fsk_lib.m:ldpc_decode() */

#include "H2064_516_sparse.h"  
#include "HRA_112_112.h"  
#include "HRAb_396_504.h"

int opt_exists(char *argv[], int argc, char opt[]) {
    int i;
    for (i=0; i<argc; i++) {
        if (strcmp(argv[i], opt) == 0) {
            return i;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    FILE         *fin, *fout;
    int           arg, sd, i, frames, codename, testframes, Nframes, data_bits_per_frame, parity_bits_per_frame;
    struct LDPC   ldpc;
    int unused_data_bits;
    
    if (argc < 2) {
        fprintf(stderr, "\n");
        fprintf(stderr, "usage: %s InputOneBytePerBit OutputFile [--sd] [--code CodeName] [--testframes Nframes] [--unused numUnusedDataBits]\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "usage: %s --listcodes\n\n", argv[0]);
        fprintf(stderr, "  List supported codes (more can be added via using Octave ldpc scripts)\n");
        fprintf(stderr, "\n");
        exit(0);
    }

    /* todo: put this in a function file to share with ldpc_dec.c */

    if ((codename = opt_exists(argv, argc, "--listcodes")) != 0) {
        fprintf(stderr,"\n");
        fprintf(stderr,"H2064_516_sparse\n");
        fprintf(stderr,"HRA_112_112\n");
        fprintf(stderr,"HRAb_396_504\n");
        fprintf(stderr,"\n");
        exit(0);
    }

    /* set up LDPC code from include file constants */

    if ((codename = opt_exists(argv, argc, "--code")) != 0) {

        /* short rate 1/2 code for FreeDV HF digital voice */

        if (strcmp(argv[codename+1], "HRA_112_112") == 0) {
            fprintf(stderr, "code: %s\n", argv[codename+1]);
            ldpc.CodeLength = HRA_112_112_CODELENGTH;
            ldpc.NumberParityBits = HRA_112_112_NUMBERPARITYBITS;
            ldpc.NumberRowsHcols = HRA_112_112_NUMBERROWSHCOLS;
            ldpc.max_row_weight = HRA_112_112_MAX_ROW_WEIGHT;
            ldpc.max_col_weight = HRA_112_112_MAX_COL_WEIGHT;
            ldpc.H_rows = (uint16_t *)HRA_112_112_H_rows;
            ldpc.H_cols = (uint16_t *)HRA_112_112_H_cols;
        }
        else if (strcmp(argv[codename+1], "HRAb_396_504") == 0) {
            fprintf(stderr, "code: %s\n", argv[codename+1]);
            ldpc.CodeLength = HRAb_396_504_CODELENGTH;
            ldpc.NumberParityBits = HRAb_396_504_NUMBERPARITYBITS;
            ldpc.NumberRowsHcols = HRAb_396_504_NUMBERROWSHCOLS;
            ldpc.max_row_weight = HRAb_396_504_MAX_ROW_WEIGHT;
            ldpc.max_col_weight = HRAb_396_504_MAX_COL_WEIGHT;
            ldpc.H_rows = (uint16_t *)HRAb_396_504_H_rows;
            ldpc.H_cols = (uint16_t *)HRAb_396_504_H_cols;
        }
        else {
            fprintf(stderr, "Unknown code: %s, defaulting to H2064_516_sparse\n", argv[codename+1]);
        }

    } else {

        /* default Wenet High Alitiude Balloon rate 0.8 code */

        ldpc.CodeLength = CODELENGTH;
        ldpc.NumberParityBits = NUMBERPARITYBITS;
        ldpc.NumberRowsHcols = NUMBERROWSHCOLS;
        ldpc.max_row_weight = MAX_ROW_WEIGHT;
        ldpc.max_col_weight = MAX_COL_WEIGHT;
        ldpc.H_rows = H_rows;
        ldpc.H_cols = H_cols;
    }
    data_bits_per_frame = ldpc.NumberRowsHcols;
    parity_bits_per_frame = ldpc.NumberParityBits;
    
    unsigned char ibits[data_bits_per_frame];
    unsigned char pbits[parity_bits_per_frame];
    double        sdout[data_bits_per_frame+parity_bits_per_frame];

    if (strcmp(argv[1], "-")  == 0) fin = stdin;
    else if ( (fin = fopen(argv[1],"rb")) == NULL ) {
        fprintf(stderr, "Error opening input bit file: %s: %s.\n",
                argv[1], strerror(errno));
        exit(1);
    }
        
    if (strcmp(argv[2], "-") == 0) fout = stdout;
    else if ( (fout = fopen(argv[2],"wb")) == NULL ) {
        fprintf(stderr, "Error opening output bit file: %s: %s.\n",
                argv[2], strerror(errno));
        exit(1);
    }
    
    sd = 0;
    if (opt_exists(argv, argc, "--sd")) {
        sd = 1;
    }

    unused_data_bits = 0;
    if ((arg = opt_exists(argv, argc, "--unused"))) {
        unused_data_bits = atoi(argv[arg+1]);
    }
    
    testframes = Nframes = 0;

    if ((arg = (opt_exists(argv, argc, "--testframes")))) {
        testframes = 1;
        Nframes = atoi(argv[arg+1]);
        fprintf(stderr, "Nframes: %d\n", Nframes);
    }

    frames = 0;
    int written = 0;
    
    while (fread(ibits, sizeof(char), data_bits_per_frame, fin) == data_bits_per_frame) {
        if (testframes) {
            uint16_t r[data_bits_per_frame];
            ofdm_rand(r, data_bits_per_frame);

            for(i=0; i<data_bits_per_frame-unused_data_bits; i++) {
                ibits[i] = r[i] > 16384;
            }
            for(i=data_bits_per_frame-unused_data_bits; i<data_bits_per_frame; i++) {
                ibits[i] = 1;
            }
           
        }
        
        encode(&ldpc, ibits, pbits);  
        
        if (sd) {
            /* map to BPSK symbols */
            for (i=0; i<data_bits_per_frame-unused_data_bits; i++)
                sdout[i] = 1.0 - 2.0 * ibits[i];
            for (i=0; i<parity_bits_per_frame; i++)
                sdout[i+data_bits_per_frame-unused_data_bits] = 1.0 - 2.0 * pbits[i];
            written += fwrite(sdout, sizeof(double), data_bits_per_frame-unused_data_bits+parity_bits_per_frame, fout); 
        }
        else {
            fwrite(ibits, sizeof(char), data_bits_per_frame, fout); 
            fwrite(pbits, sizeof(char), parity_bits_per_frame, fout); 
        }
        
        frames++;       
        if (testframes && (frames >= Nframes)) {
            goto finished;
        }
    }

 finished:

    fprintf(stderr, "written: %d\n", written);
    fclose(fin);  
    fclose(fout); 

    return 1;
}