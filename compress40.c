/* Last Modified: March 5th, 2020
 * Brian Savage and Vichka Fonarev
 * bsavag01         vfonar01
 * 
 * Comp40 - HW4 - Arith
 * bitpack.c
 * Does: compresses and decompresses ppm images
 * Uses: -c [ppm image to compress] -> outputs a COMP40 Compressed image 
 *                                                              format 2
 *       -d [COMP40 Compressed image format 2] -> outs a decompressaed
 *                                                           ppm image
 * Possible Errors: Handles all unit testing and (de)compression properly
 */

#include <stdio.h>
#include <stdlib.h>
#include "uarray2.h"
#include "uarray.h"
#include "assert.h"
#include "pnm.h"
#include "math.h"
#include "a2plain.h"
#include "compress40.h"
#include "bitpack.h"
#include "arith40.h"

#define UNSIGNED uint64_t
#define SIGNED int64_t
#define SIZE 64

#define PR_LSB 0
#define PB_LSB 4
#define D_LSB 8
#define C_LSB 14
#define B_LSB 20
#define A_LSB 26

#define PR_WIDTH 4
#define PB_WIDTH 4
#define D_WIDTH 6
#define C_WIDTH 6
#define B_WIDTH 6
#define A_WIDTH 6

#define A_SCALE 63 // new for challenge prev 511
#define BCD_SCALE 100 // new for challenge prev 50

/*
* holds the smallest dimesnsion of two images
* two integers width, height
*/
struct dimensions {
    int width;
    int height;
}; typedef struct dimensions dimensions;

/* struct that holds info needed to represent a dct_ transfrom */
/* four floats */
struct dct_representation {
    float a;
    float b;
    float c;
    float d;
}; typedef struct dct_representation dct_representation;

/* struct that holds info needed to represent a pixel in component form */
struct component_color {
    float Y;
    float P_B;
    float P_R;
}; typedef struct component_color component_color;

/* custom closure argument struct to hold ppm image and dimension struct*/
struct image_closure {
    Pnm_ppm image;
    dimensions dimension;
}; typedef struct image_closure *image_closure;

/* word analog that holds all data that needs to be packed into a word
    in explicit form */
struct word_analog {
    int P_B, P_R, a, b, c, d;
}; typedef struct word_analog word_analog;

/* 
 * Element (i, j) in the world of ideas maps to
 * rows[j][i] where the square brackets stand for access
 * to a Hanson UArray_T
 */
#define T UArray2_T
struct T {
        int width, height;
        int size;
        UArray_T rows; /* UArray_T of 'height' UArray_Ts,
                          each of length 'width' and size 'size' */
};

struct dimensions even_out_dimensions(T array2D);

Pnm_ppm read_image(FILE* file, A2Methods_T methods);

component_color rgb_to_component(Pnm_rgb pixel, float denominator);
struct Pnm_rgb component_to_rgb(component_color pixel, float denominator);

void compress(int col, int row, T array2D, void *elem, void *cl);
void decompress(int col, int row, T array2D, void *elem, void *cl);

dct_representation pixel_to_dct(component_color pix_one, 
                                component_color pix_two,
                                component_color pix_three,
                                component_color pix_four);
void dct_to_pixel(dct_representation* values);
void quantize_dct(dct_representation* dct_representation);
float sum_brightness(component_color pix_one, 
                     component_color pix_two,
                     component_color pix_three,
                     component_color pix_four);

word_analog pack_word(component_color pix_one , component_color pix_two,
               component_color pix_three, component_color pix_four);
void print_word(word_analog word);
word_analog unpack_word(UNSIGNED packed_word);
UNSIGNED read_word(FILE *file);

/******************************************************************************
 *                              compress40
 *
 *  reads in a ppm image and prints out a compressed version in the compress40
 *  format to stdout
 *  
 * PARAMS   :   FILE* file  : open file pointer to compressed image
 * 
 * RETURNS  :   void
 * 
 * ***************************************************************************/
void compress40  (FILE* input) 
{
    assert(input != NULL); /*makes sure file is not NULL*/
    
    A2Methods_T methods = uarray2_methods_plain; 
    assert(methods != NULL);

    Pnm_ppm image = read_image(input, methods);
    assert(image != NULL);

    /* rounds dimensions to neareset smallest even number and stores
                    it in a dimension struct*/
    dimensions dimension = even_out_dimensions(image -> pixels);

    /* mallocs space for closure arguemnt that holds image and dimensions */
    image_closure image_struct = malloc(sizeof(struct image_closure));
    assert(image_struct != NULL);

    image_struct -> image = image;
    image_struct -> dimension = dimension;

    /* header for compressed image format */
    printf("COMP40 Compressed image format 2\n%u %u\n", 
                            dimension.width, dimension.height);

    /* compressed and prints image through map function */
    UArray2_map_row_major(image -> pixels, compress, image_struct);

    /* free memory */
    Pnm_ppmfree(&(image_struct -> image));
    free(image_struct);
}

/******************************************************************************
 *                             decompress40
 *
 *  reads in a compressed image from the already opened file, and prints out
 *  a ppm image on standard out 
 *  
 * PARAMS   :   FILE* file  : open file pointer to compressed image
 * 
 * RETURNS  :   void
 * 
 * ***************************************************************************/
void decompress40(FILE* input)  
{
    assert(input != NULL);

    A2Methods_T methods = uarray2_methods_plain; /*method suite*/

    /*read in header*/
    unsigned width, height;
    unsigned read = fscanf(input, "COMP40 Compressed image format 2\n%u %u",
                                                        &width, &height);    
                                            
    assert(read == 2);
    char c = getc(input);
    assert(c == '\n'); /* checks for proper formatting in image */

    /* create a 2d array based on the dimensions read in from the
                        compressed file                         */
    UArray2_T array2D = UArray2_new(width, height, sizeof(struct Pnm_rgb));
    assert(array2D != NULL);

    /* packs image struct */
    struct Pnm_ppm image = {
        .width = width,
        .height = height,
        .denominator = 255,
        .pixels = array2D,
        .methods = methods
    };

    /* calls decompress on compressed file withmap function*/
    UArray2_map_row_major(image.pixels, decompress, input);

    /* print image */
    Pnm_ppmwrite(stdout, &image);
    UArray2_free(&array2D);
}

/******************************************************************************
 *                              compress
 *
 * map function for compressing an image, after each 2x2 block is compressed
 * it is printed to stdout
 *  
 * PARAMS   :   int col     : column value of current element
 *              int row     : row value of current element
 *              T array2d   : 2D array holding pixels
 *              void* elem  : current element (not used)
 *              void* cl    : closure argument, needs to be a file
 * 
 * RETURNS  :   void
 * 
 * ***************************************************************************/
void compress(int col, int row, T array2D, void *elem, void *cl) 
{
    (void) elem;

    assert(cl);
    assert(array2D);
    
    image_closure image_struct = (image_closure)cl; 
    dimensions dimension = image_struct -> dimension;

    /* current position is inside 'safe' dimensions*/
    if (col < dimension.width && row < dimension.height) {
        /* at bottome right of 2x2 square: now have access to previous
                    four pixels in the 'block                          */
        if (col % 2 == 1 && row % 2 == 1) {
            /* convert top left pix to component form */
            component_color pix_one     =   rgb_to_component(
                                            (Pnm_rgb)UArray2_at
                                            (array2D, col - 1, row - 1),
                                        image_struct -> image -> denominator);

            /* convert top right pix to component form */
            component_color pix_two     =   rgb_to_component(
                                            (Pnm_rgb)UArray2_at
                                            (array2D, col, row - 1),
                                        image_struct -> image -> denominator);
            
            /* convert bottom left pix to component form */
            component_color pix_three   =   rgb_to_component(
                                            (Pnm_rgb)UArray2_at
                                            (array2D, col - 1, row),
                                        image_struct -> image -> denominator);

            /* convert bottom right pix to component form */
            component_color pix_four    =   rgb_to_component(
                                            (Pnm_rgb)UArray2_at
                                            (array2D, col, row),
                                        image_struct -> image -> denominator);

            /* create packed word ananlog from four pixels */
            word_analog word = pack_word(pix_one, pix_two, pix_three, 
                                         pix_four);

            print_word(word); /*pack and print word to sdout */
        }
    }       
}    

/******************************************************************************
 *                              decompress
 *
 * map function for decompressing an image, builds a image into the array
 * parameter, from the file passed in the closure arg
 *  
 * PARAMS   :   int col     : column value of current element
 *              int row     : row value of current element
 *              T array2d   : 2D array holding pixels
 *              void* elem  : current element (not used)
 *              void* cl    : closure argument, expected to be a FILE* to
 *                            an open file
 * 
 * RETURNS  :   void
 * 
 * ***************************************************************************/
void decompress(int col, int row, T array2D, void *elem, void *cl) {
    (void) elem;
    assert(cl != NULL);
    assert(array2D != NULL);

    FILE* file = (FILE*) cl;
    if (col % 2 == 1 && row % 2 == 1) { /* bottom right of 2x2 */

        UNSIGNED packed_word = read_word(file); /* read in word from file */
        
        /* extract data from packed word*/
        word_analog word = unpack_word(packed_word);

        dct_representation dct_rep = {
            .a = word.a,
            .b = word.b,
            .c = word.c,
            .d = word.d
        };

        /* undo discrete cosign transfrom */
        dct_to_pixel(&dct_rep);

        /* converts index of chroma to chroma */
        float P_B = Arith40_chroma_of_index(word.P_B);
        float P_R = Arith40_chroma_of_index(word.P_R);
        /* converts index of chroma to chroma */

        /* creates 4 pixels in component form from the word read in */
        component_color pix_one     = { dct_rep.a, P_B, P_R };
        component_color pix_two     = { dct_rep.b, P_B, P_R };
        component_color pix_three   = { dct_rep.c, P_B, P_R };
        component_color pix_four    = { dct_rep.d, P_B, P_R };
        /* creates 4 pixels in component form from the word read in */
        
        /* converts component pixels into Pnm_rgb pixels and assignes them
                    to the proper place in the 2d array                 */
        *(Pnm_rgb)UArray2_at(array2D, col - 1, row - 1) = 
                                            component_to_rgb(pix_one, 255);
        *(Pnm_rgb)UArray2_at(array2D, col, row - 1)     = 
                                            component_to_rgb(pix_two, 255);
        *(Pnm_rgb)UArray2_at(array2D, col - 1, row)     = 
                                            component_to_rgb(pix_three, 255);
        *(Pnm_rgb)UArray2_at(array2D, col, row)         = 
                                            component_to_rgb(pix_four, 255);
    }
}

/******************************************************************************
 *                              pack_word
 *  takes data from 4 pixels in component forms, compresses them with a DCT 
 *  transform and quantization and the puts that info in a word_analog struct 
 * 
 * PARAMS   :   component_color pix_one     : pixel in component form
 *          :   component_color pix_two     : pixel in component form
 *          :   component_color pix_three   : pixel in component form
 *          :   component_color pix_four    : pixel in component form
 * 
 * RETURNS  :   word_analog             : struct holding data extracted from
 *                                        packed word
 * ***************************************************************************/
word_analog pack_word(component_color pix_one , component_color pix_two,
               component_color pix_three, component_color pix_four) 
{
    word_analog word;
    dct_representation dct_transform = pixel_to_dct(pix_one, pix_two, 
                                                    pix_three, pix_four);
    quantize_dct(&dct_transform);
    
    float average_Pb = (pix_one.P_B + pix_two.P_B + pix_three.P_B + 
                        pix_four.P_B) / 4.0; 
    float average_Pr = (pix_one.P_R + pix_two.P_R + pix_three.P_R + 
                        pix_four.P_R) / 4.0;
    
    word.a = (int) dct_transform.a;
    word.b = (int) dct_transform.b;   
    word.c = (int) dct_transform.c;   
    word.d = (int) dct_transform.d;

    word.P_B = (int) Arith40_index_of_chroma(average_Pb);
    word.P_R = (int) Arith40_index_of_chroma(average_Pr);

    return word;
}

/******************************************************************************
 *                          print_word
 *
 * Takes the contents of a word_analog struct, packs it into a word and prints
 * in big-endian order
 * 
 * PARAMS   :   word_analog word    : struct holding all data on compressed
 *                                    2x2 pixel block
 * 
 * RETURNS  :   void
 * 
 * ***************************************************************************/
void print_word(word_analog word)
{
    UNSIGNED packed = 0;
    /* packs word by the values contained with the word analog struct */
    packed = Bitpack_newu(packed, A_WIDTH, A_LSB, word.a);
    packed = Bitpack_news(packed, B_WIDTH, B_LSB, word.b);
    packed = Bitpack_news(packed, C_WIDTH, C_LSB, word.c); 
    packed = Bitpack_news(packed, D_WIDTH, D_LSB, word.d);
    packed = Bitpack_newu(packed, PR_WIDTH, PR_LSB, word.P_R);
    packed = Bitpack_newu(packed, PB_WIDTH, PB_LSB, word.P_B);

    for (int i = 3; i > -1; i--) { /* writes to file in Big Endian */
        putchar(Bitpack_getu(packed, 8, i * 8));
    }
}

/******************************************************************************
 *                          unpack_word
 * 
 *  uses bitpack to unpack a packed 32bit word and put the contents in a
 *  word_analog struct
 * 
 * PARAMS   :   UNSIGNED packed_word    : packed 32bit word holding image data
 * 
 * RETURNS  :   word_analog             : struct holding data extracted from
 *                                        packed word
 * ***************************************************************************/
word_analog unpack_word(UNSIGNED packed_word) 
{
    word_analog word;
    /* Unpacks from the packed_word and returns the initializes 
     * values of the word_analog struct */
    word.a   = (int) Bitpack_getu(packed_word, A_WIDTH, A_LSB);
    word.b   = (int) Bitpack_gets(packed_word, B_WIDTH, B_LSB);
    word.c   = (int) Bitpack_gets(packed_word, C_WIDTH, C_LSB);
    word.d   = (int) Bitpack_gets(packed_word, D_WIDTH, D_LSB);
    word.P_B = (int) Bitpack_getu(packed_word, PB_WIDTH, PB_LSB);  
    word.P_R = (int) Bitpack_getu(packed_word, PR_WIDTH, PR_LSB);

    return word;
}

/******************************************************************************
 *                          read_word
 * 
 *  reads word in big endian format from pre-opened file
 * 
 * PARAMS   :   FILE* file      :   pointer to open file, should contain
 *                                  'words' representing  a compressed
 *                                  image
 * 
 * RETURNS  :   UNSIGNED        : 32 bit word read in from file
 * ***************************************************************************/
UNSIGNED read_word(FILE *file)
{
    assert(file != NULL);
    UNSIGNED word = 0;
    for (int i = 3; i > -1; i--) { /* reads from Big Endian to recreate word */
        word = Bitpack_newu(word, 8, i * 8, (UNSIGNED) fgetc(file));
    }
    return word;
}

/******************************************************************************
 *                          quantize_dct
 * 
 *  scales decimal values to fit in appropropriate integers for word packing,
 *  doubles as quantization of sorts, by scaling values
 * 
 * PARAMS   :   dct_representation* dct_representation  : struct holding four
 *                                                        float values that
 *                                                        represent the dct
 *                                                        transformed data
 * 
 * RETURNS  :   void
 * ***************************************************************************/
void quantize_dct(dct_representation* dct_representation) 
{
    assert(dct_representation != NULL);

    dct_representation -> a = dct_representation -> a * A_SCALE; 
    /* ^ scale to fit 9 bits */
    dct_representation -> b = dct_representation -> b * BCD_SCALE;  
    /* ^ scale to fit into signed 5 bit int */
    dct_representation -> c = dct_representation -> c * BCD_SCALE;  
    /* ^ scale to fit into signed 5 bit int */
    dct_representation -> d = dct_representation -> d * BCD_SCALE;  
    /* ^ scale to fit into signed 5 bit int */
}

/******************************************************************************
 *                             rgb_to_component 
 * 
 *  converts pixel from rgb representation to component form
 * 
 * PARAMS   :   Pnm_rgb pixel           :   pointer to Pnm rgb representation
 *                                          of a pixel
 *              float denominator       :   max value of pixel
 * 
 * RETURNS  :   compnent_color pixel    :   pixel in component representation
 * ***************************************************************************/
component_color rgb_to_component(Pnm_rgb pixel, float denominator) 
{
    assert(pixel != NULL);

    component_color converted_pix;

    float red = pixel -> red / denominator;
    float green = pixel -> green / denominator;
    float blue = pixel -> blue / denominator;
    
    converted_pix.Y = (0.299 * red) + (0.587 * green) + (0.114 * blue);
    converted_pix.P_B = (-0.168736 * red) - (0.331264 * green) + (0.5 * blue);
    converted_pix.P_R = (0.5 * red) - (0.418688 * green) - (0.081312 * blue);

    return converted_pix;
}

/******************************************************************************
 *                          component_to_rgb 
 * 
 *  converts pixel from component represntationg to r, g, b representation
 * 
 * PARAMS   :   compnent_color pixel    :   pixel in component representation
 *              float denominator       :   max value of pixel
 * 
 * RETURNS  :   struct Pnm_rgb          :   Pnm_rgb struct holding r,g,b values
 *                                          of a pix
 * ***************************************************************************/
struct Pnm_rgb component_to_rgb(component_color pixel, float denominator) 
{
    struct Pnm_rgb converted_pix;
    
    float Y = pixel.Y;
    float P_B = pixel.P_B;
    float P_R = pixel.P_R;

    int red = (Y + 1.402 * P_R) * denominator;
    int green = (Y - 0.344136 * P_B - 0.714136 * P_R) * denominator;
    int blue = (Y + 1.772 * P_B) * denominator;

    converted_pix.red    = fmax(fmin(red, denominator), 0);
    converted_pix.green  = fmax(fmin(green, denominator), 0);
    converted_pix.blue   = fmax(fmin(blue , denominator), 0);

    return converted_pix;
}

/******************************************************************************
 *                           pixel_to_dct
 * 
 *  takes 4 pixels and transfroms them using the discrete cosine
 *  transform
 * 
 * PARAMS   :   compnent_color pix_one      : pixel in component representation
 *          :   compnent_color pix_two      : pixel in component representation
 *          :   compnent_color pix_three    : pixel in component representation
 *          :   compnent_color pix_four     : pixel in component representation
 * 
 * RETURNS  :   dct_representation  : structing holding the 4 floats
 *                                    representing the dct'ed pixels
 * ***************************************************************************/
dct_representation pixel_to_dct(component_color pix_one, 
                                component_color pix_two,
                                component_color pix_three,
                                component_color pix_four) 
{
    dct_representation converted_values;

                    /* average brightness of 4 pixels */
    converted_values.a = (pix_one.Y + pix_two.Y + 
                          pix_three.Y + pix_four.Y) / 4.0;
    if (converted_values.a > 1) {converted_values.a = 1;}
    if (converted_values.a < 0) {converted_values.a = 0;}
   
    converted_values.b = ((pix_three.Y + pix_four.Y) - 
                          (pix_one.Y + pix_two.Y)) / 4.0;
    if (converted_values.b < -0.3) { converted_values.b = -0.3;}
    if (converted_values.b > 0.3) { converted_values.b = 0.3;}

    converted_values.c = ((pix_two.Y + pix_four.Y) - 
                          (pix_one.Y + pix_three.Y)) / 4.0;
    if (converted_values.c < -0.3) { converted_values.c = -0.3;}
    if (converted_values.c > 0.3) { converted_values.c = 0.3;}
    
    converted_values.d = ((pix_one.Y + pix_four.Y) - 
                          (pix_two.Y + pix_three.Y))/ 4.0;
    if (converted_values.d < -0.3) { converted_values.d = -0.3;}
    if (converted_values.d > 0.3) { converted_values.d = 0.3;} 
    
    return converted_values;
}

/******************************************************************************
 *                                dct_to_pixel
 * 
 *  converts discrete cosine transform represnatation of a pixel
 *  into a standard rgb representation (in place) aka undoes it
 * 
 * PARAMS   :   dct_representation* pixel   : pointer to struct holding
 *                                            4 floats : dct rep
 * 
 * RETURNS  :   void
 * 
 * ***************************************************************************/
void dct_to_pixel(dct_representation* values) 
{
    assert(values != NULL);

    float a = values -> a / A_SCALE;
    float b = values -> b / BCD_SCALE; 
    float c = values -> c / BCD_SCALE; 
    float d = values -> d / BCD_SCALE; 
    
    values -> a = a - b - c + d;
    values -> b = a - b + c - d;
    values -> c = a + b - c - d;
    values -> d = a + b + c + d;
}

/******************************************************************************
 *                            even_out_dimensions
 * 
 *  rounds dimensions of an image to the nearest evennumber 
 * 
 * PARAMS   :   UArray2_T array_one :   2D array with all info
 *          :   UArray2_T array_two :   2D array with all info
 * 
 * RETURNS  :   dimensions          :   struct that holds the 'evanized'
 *                                      dimensions
 * ***************************************************************************/
dimensions even_out_dimensions(T array2D) 
{
    assert(array2D != NULL);

    dimensions new_dimensions;
    new_dimensions.width = ((array2D -> width) / 2) * 2;
    new_dimensions.height = ((array2D -> height) / 2) * 2;

    return new_dimensions;
}

/******************************************************************************
 *                          read_image
 * 
 *  reads in image from an opened file
 * 
 * PARAMS   :   FILE* file      :   pointer to open file, should contain 
 *                                  ppm img
 *              A2Methods_T methods : method suite needed by 2d array impl
 * 
 * RETURNS  :   Pnm_ppm image   : image provided on command line
 * ***************************************************************************/
Pnm_ppm read_image(FILE* file, A2Methods_T methods) 
{
    assert(methods != NULL);
    assert(file != NULL);

    return Pnm_ppmread(file, methods);
}
