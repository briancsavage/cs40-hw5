#include <stdio.h>
#include <stdlib.h>
#include "uarray2.h"
#include "uarray.h"
#include "assert.h"
#include "pnm.h"
#include "math.h"
#include "a2plain.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b));
#endif

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

/*
* holds the smallest dimesnsion of two images
* two integers width, height
*/
struct dimensions {
    int width;
    int height;
};

FILE* openFile(char* filename)
{
    if (filename != NULL) {
    return fopen(filename, "rb");
    }
    return NULL;
}

float computeDiff(UArray2_T image_one_array, UArray2_T image_two_array, int denom_one, int denom_two);

int main(int argc, char *argv[])  {
    A2Methods_T methods = uarray2_methods_plain; 
    assert(methods != NULL);
    
    if (argc < 3) {
        fprintf(stderr, "Not enough command line argiuuiuments");
        exit(EXIT_FAILURE);
    } else if (argc == 3) {
        char* image_one_name = argv[1];
        char* image_two_name = argv[2];

        /* open image files and make sure they were opened properly */
        FILE* image_one_file = openFile(image_one_name);
        FILE* image_two_file = openFile(image_two_name); 

        assert(image_one_file != NULL);
        assert(image_two_file != NULL);
        /* open image files and make sure they were opened properly */

        /* read in images */
        Pnm_ppm image_one = Pnm_ppmread(image_one_file, methods);
        assert(image_one != NULL);
        
        Pnm_ppm image_two = Pnm_ppmread(image_two_file, methods);
        assert(image_two != NULL);
        /* read in images */

        /* compute difference of two images */
        float difference = computeDiff(image_one -> pixels, image_two -> pixels, image_one -> denominator, image_two -> denominator);

        /* print with 4 decimal places */
        printf("%0.4f \n", difference);
    } else {
        fprintf(stderr, "Too many command line arguments");
        exit(EXIT_FAILURE);
    }
}

/**********************************************
 *          calculate_smallest_dimension
 * 
 * calcualtes the smallest width and height of two arrays
 * and creates + returns a struct that containts the smallest
 * width and smallest height
 * 
 * PARAMS   :   UArray2_T array_one :   2D array with all info
 *          :   UArray2_T array_two :   2D array with all info
 * 
 * RETURNS  :   dimensions          :   struct that holds the smaller
 *                                      of the two dimnestions
 * *******************************************/
struct dimensions calculate_smallest_dimension(UArray2_T array_one, UArray2_T array_two) {
    struct dimensions min_dimensions;

    min_dimensions.width = min(array_one -> width, array_two -> width);
    min_dimensions.height = min(array_one -> height, array_two -> height);

    return min_dimensions;
}

/**********************************************
 *               computeDiff
 * 
 * computes the difference in pixel values of two ppm images returning the
 * percent difference as a float 
 * 
 * PARAMS   :   UArray2_T array_one :   2D array with all info
 *          :   UArray2_T array_two :   2D array with all info
 * 
 * RETRUNS  :   float               :   float represnatation of
 *                                     
 * *******************************************/
float computeDiff(UArray2_T image_one_array, UArray2_T image_two_array, int denom_one, int denom_two)
{
    /* make sure parameters are not null*/
    assert(image_one_array != NULL);
    assert(image_two_array != NULL);
    /* make sure parameters are not null*/

    assert(image_one_array -> size == image_two_array -> size);

    float diff = 0;

    /* calculates minimum dimesnions */
    struct dimensions min_dimensions = calculate_smallest_dimension(image_one_array, image_two_array);
    
    double numerator = 0;
    double denominator = 3 * min_dimensions.width * min_dimensions.height;

    for (int row = 0; row < min_dimensions.height; row++) {
        for (int col = 0; col < min_dimensions.width; col++) {
            Pnm_rgb origPix = (Pnm_rgb) UArray2_at(image_one_array, col, row);
            Pnm_rgb compPix = (Pnm_rgb) UArray2_at(image_two_array, col, row);

            double redOne      = (origPix -> red) / (double) denom_one;
            double greenOne    = (origPix -> green) / (double) denom_one;
            double blueOne     = (origPix -> blue) / (double) denom_one;

            double redTwo      = (compPix -> red) / (double) denom_two;
            double greenTwo    = (compPix -> green) / (double) denom_two;
            double blueTwo     = (compPix -> blue) / (double) denom_two;
            
            /* sums squares of differnece of each */
            numerator += (redOne - redTwo) * (redOne - redTwo) + (greenOne - greenTwo)
                  * (greenOne - greenTwo) + (blueOne - blueTwo) * (blueOne - blueTwo);
        }
    }
    
    diff = sqrt(numerator / denominator);
    return diff;
}