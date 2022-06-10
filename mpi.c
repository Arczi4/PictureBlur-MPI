#include <stdio.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int min = -2, max = 3;

typedef struct {
    int blue;
    int green;
    int czerw;
}Pixel;

int main(int argc, char** argv) {
    int width, height, channels;
    unsigned char *img = stbi_load("potezny.jpg", &width, &height, &channels, 0);
    if (img == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

    int blur_channels = channels == 4 ? 2 : 1;
    int counter = 0;
    size_t img_size = width * channels * height;

    Pixel* pixels = malloc(sizeof(Pixel) * width * height);
    Pixel* blur = malloc(sizeof(Pixel) * width * height);

    // loop for adding collors to the pixel table
    for (unsigned char* p = img; p != img + img_size; p += channels) {
        pixels[counter].czerw = (uint8_t)(*p);
        pixels[counter].green = (uint8_t)(*(p + 1));
        pixels[counter].blue = (uint8_t)(*(p + 2));
        counter++;
    }
    int rank, n_ranks;

    int avg_red = 0, avg_green = 0, avg_blue = 0;
    int c_r = 0;
    int c_g = 0;
    int c_b = 0;

    // First call MPI_Init
    
    MPI_Init(&argc, &argv);
    // Check that there are two ranks
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);
    if (n_ranks != 3) {
        printf("This example requires exactly three ranks\n");
        MPI_Finalize();
        return(1);
    }
    
    // Get my rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double start = MPI_Wtime();

    if (rank == 0) {
        for (int y = 0; y < height; y++){
            for (int x = 0; x < width; x++){
                for (int i = min; i < max; i++){ // horizontal
                    for (int j = min; j < max; j++){ // vertical
                        if ((y + i < height && y + i >= 0) && (x + j < width && x + j >= 0)){
                            avg_red += pixels[(y + i) * width + x + j].czerw;
                            c_r++;
                        }
                    }
                }
                blur[y * width + x].czerw = avg_red / c_r;
                c_r = 0;
                avg_red = 0;
            }
        }
        //MPI_Send(blur->czerw, 16, MPI_CHAR, 3, 0, MPI_COMM_WORLD);
    }

    if (rank == 1) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                for (int i = min; i < max; i++) { 
                    for (int j = min; j < max; j++) { 
                        if ((y + i < height && y + i >= 0) && (x + j < width && x + j >= 0)) {
                            avg_green += pixels[(y + i) * width + x + j].green;
                            c_g++;
                        }
                    }
                }
                blur[y * width + x].green = avg_green / c_g;
                c_g = 0;
                avg_green = 0;
            }
        }
        MPI_Send(blur->green, 16, MPI_CHAR, 3, 0, MPI_COMM_WORLD);
    }

    if (rank == 2) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                for (int i = min; i < max; i++) { 
                    for (int j = min; j < max; j++) { 
                        if ((y + i < height && y + i >= 0) && (x + j < width && x + j >= 0)) {
                            avg_blue += pixels[(y + i) * width + x + j].blue;
                            c_b++;
                        }
                    }
                }
                blur[y * width + x].blue = avg_blue / c_b;
                c_b = 0;
                avg_blue = 0;
            }
        }
    }



    double end = MPI_Wtime();
    MPI_Finalize();

    double time = end-start;
    printf("Blur done!\n");
    printf("time: %f [s]\n\n", time);

    unsigned char* res = malloc(width * height * 3);

    int index = 0;

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            res[index++] = (unsigned char)blur[j * width + i].czerw;
            res[index++] = (unsigned char)blur[j * width + i].green;
            res[index++] = (unsigned char)blur[j * width + i].blue;

        }
    }
    
    stbi_write_jpg("blur_pic.jpg", width, height, 3, res, 100);
    stbi_image_free(img);
    free(pixels);
    free(res);
    free(blur);
}