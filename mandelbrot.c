#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

/* Structs */
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint32_t reserved1;
    uint32_t offset;
} BmpFileHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imagesize;
    int32_t xresolution;
    int32_t yresolution;
    uint32_t ncolors;
    uint32_t importantcolors;
} BmpInfoHeader;
#pragma pack(pop)

typedef struct {
    double complexx;
    double complexy;
    int row;
    int column;
} Input;

/* Global Variables */
int img_dim; // width and height of image, in pixels
int engine_count; // how many engines we create
double UL_X; // the top left corner x coordinate for the complex plane
double UL_Y; // the top left corner y coordinate for the complex plane
double mandel_dim; // the width/height of the mandelbrot set, such that the bottom
                   // right corner is (UL_X + mandel_dim, UL_Y + mandel_dim)

int* rowToPrint; // the current row we are working on that is set to print in the file
int rowCounter; // the current row we are working on (0 until img_dim - 1)
int colsWritten = 0; // the number of columns written to the file (0 until img_dim - 1)
int* indices;
Input* inputs; // the inputs for the engines, supplied by column threads
pthread_t* engines; // array of engines
pthread_t* cols; // array of column threads
pthread_t writer;

pthread_mutex_t* col_mutex; // mutex for columns
sem_t* engine_sem; // mutex for engines
sem_t* column_sem; // mutex for columns
pthread_mutex_t pixel_mutex; // mutex for pixel array
sem_t writer_sem; // mutex for writer

pthread_barrier_t col_and_write; // barrier for columns and writer

FILE* bmpFile; // the file to write to (mandelbrot.bmp)

/* Method Headers */
void createFile(const char* filename);
void initialize_arrays();
void* engine_thread(void* arg);
void* column_thread(void* arg);
void* print_thread(void* arg);
void free_everything(char* func);
void destroy_everything();

/**
 * Main Method: takes in the arguments and begins the process of creating all the necessary variables.
 * @param argc: the number of arguments
 * @param argv: the arguments
 * 
 * 
 * 
*/
int main(int argc, char **argv)  {
    if (argc < 6) {
        printf("Invalid input, format is: mandelbrot <img_dim> <engines> <UL_X> <UL_Y> <mandel_dim>\n");
        return 1;
    }

    /* Setting Globals */
    img_dim = atoi(argv[1]);
    engine_count = atoi(argv[2]);
    UL_X = strtod(argv[3], NULL);
    UL_Y = strtod(argv[4], NULL);
    mandel_dim = strtod(argv[5], NULL);

    rowCounter = img_dim;

    // create the bitmap file
    createFile("mandeloutput.bmp");
    initialize_arrays();

    /* Instantiating all engine threads */
    indices = malloc(sizeof(int) * engine_count);
    if (indices == NULL) free_everything("indices");

    /* Initializing barrier */
    pthread_barrier_init(&col_and_write, NULL, img_dim + 1);
    sem_init(&writer_sem, 0, 0);
    pthread_mutex_init(&pixel_mutex, NULL);

    // initializing all engine semaphores
    for (int i = 0; i < engine_count; i++) {
        sem_init(&engine_sem[i], 0, 0);
    }

    /* Initializing all column mutexes */
    for (int i = 0; i < engine_count; i++) {
        pthread_mutex_init(&col_mutex[i], NULL);
    }

    for (int i = 0; i < engine_count; i++) {
        sem_init(&column_sem[i], 0, 0);
    }

    /* Put the above two together*/
    for (int i = 0; i < engine_count; i++) {
        indices[i] = i;
        pthread_t x = engines[i];
        pthread_create(&x, NULL, engine_thread, &indices[i]);
    }

    /* Put the cols together*/
    for (int i = 0; i < img_dim; i++) {
        indices[i] = i;
        pthread_t x = cols[i];
        pthread_create(&x, NULL, column_thread, &indices[i]);
    }

    pthread_create(&writer, NULL, print_thread, NULL);

    /* Wait for all threads to finish */
    for (int i = 0; i < engine_count; i++) {
        pthread_join(engines[i], NULL);
    }
    for (int i = 0; i < img_dim; i++) {
        pthread_join(cols[i], NULL);
    }
    pthread_join(writer, NULL);

    destroy_everything();
    free_everything("nothing");
    return 0;
}

void* engine_thread(void* arg) {
    int index = *((int*) arg);
    while (rowCounter > 0) {
        sem_wait(&engine_sem[index]);
        double complex_x = inputs[index].complexx; // get vars
        double complex_y = inputs[index].complexy; // get vars

        int iteration = 0; // iterations num
        int max_iteration = 255; // max iterations
        double x = 0.0, y = 0.0; // starting vals for x and y
        double xtemp; // temp x instance
        while (x * x + y * y <= 4 && iteration < max_iteration) {
            xtemp = x * x - y * y + complex_x;
            y = 2 * x * y + complex_y;
            x = xtemp;
            iteration++;
        }
        int answer = max_iteration - iteration;

        pthread_mutex_lock(&pixel_mutex);
        colsWritten++;
        if (colsWritten == img_dim) {
            sem_post(&writer_sem);
        }
        rowToPrint[inputs[index].column] = answer;
        pthread_mutex_unlock(&pixel_mutex);
        sem_post(&column_sem[index]);
    }
    pthread_exit(NULL);
}

void *column_thread(void* arg) {

    int col = *((int*) arg);

    while (rowCounter > 0) {
        double complex_x = (double)col / img_dim * mandel_dim + UL_X;
        double complex_y = (double)rowCounter / img_dim * mandel_dim + UL_Y;
        srand(rowCounter + col);
        int index = rand() % engine_count; // get engine
        
        // lock off crit code
        pthread_mutex_lock(&col_mutex[index]);
        
        inputs[index].complexx = complex_x; // set complex
        inputs[index].complexy = complex_y; // set complex
        inputs[index].row = rowCounter; // set row
        inputs[index].column = col; // set col

        sem_post(&engine_sem[index]); // let engine do it's thing
        sem_wait(&column_sem[index]); // wait for engine to finish
        pthread_mutex_unlock(&col_mutex[index]); // free mutex up
        pthread_barrier_wait(&col_and_write); // wait for all columns to finish
    }
    pthread_exit(NULL);
}

void *print_thread(void* arg) {
    
    while (rowCounter > 0) {
        sem_wait(&writer_sem);

        for (int i = 0; i < img_dim; i++) {
            uint8_t b = rowToPrint[i] & 0xFF;
            uint8_t g = rowToPrint[i] & 0xFF;
            uint8_t r = rowToPrint[i] & 0xFF;
            fwrite(&b, 1, 1, bmpFile);
            fwrite(&g, 1, 1, bmpFile);
            fwrite(&r, 1, 1, bmpFile);
        }

        rowCounter--;
        colsWritten = 0;
        pthread_barrier_wait(&col_and_write); // wait for all columns to finish
    }
    fclose(bmpFile);
    printf("Bitmap file created successfully!\n");
    pthread_exit(NULL);
    return arg;
}

/**
 * Method to create just the file and set the headers
 * @param filename: the name of the file to be created
*/
void createFile(const char* filename) {
    BmpFileHeader fileHeader = { 0x4D42, sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + img_dim * img_dim * 3, 0, sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) };
    BmpInfoHeader infoHeader = { sizeof(BmpInfoHeader), img_dim, img_dim, 1, 24, 0, img_dim * img_dim * 3, 0, 0, 0, 0 };

    bmpFile = fopen(filename, "wb");
    if (!bmpFile) {
        perror("Error opening file");
        return;
    }

    // Write headers
    fwrite(&fileHeader, sizeof(BmpFileHeader), 1, bmpFile);
    fwrite(&infoHeader, sizeof(BmpInfoHeader), 1, bmpFile);
}

void initialize_arrays() {
    /* Allocating row-pixel array */
    rowToPrint = (int*) malloc(img_dim * sizeof(int));
    if (rowToPrint == NULL) free_everything("rowToPrint");

    /* Allocating engine struct array */
    inputs = (Input*) malloc(engine_count * sizeof(Input));
    if (inputs == NULL) free_everything("inputs");

    /* Allocating column array */
    cols = (pthread_t*) malloc(img_dim * sizeof(pthread_t));
    if (cols == NULL) free_everything("cols");

    /* Allocating column mutex array */
    col_mutex = (pthread_mutex_t*) malloc(engine_count * sizeof(pthread_mutex_t));
    if (col_mutex == NULL) free_everything("col_mutex");

    column_sem = (sem_t*) malloc(engine_count * sizeof(pthread_mutex_t));
    if (column_sem == NULL) free_everything("column_sem");

    /* Allocating engine mutex array */
    engine_sem = (sem_t*) malloc(engine_count * sizeof(pthread_mutex_t));
    if (engine_sem == NULL) free_everything("engine_mutex");

    /* Allocating engine array */
    engines = (pthread_t*) malloc(engine_count * sizeof(pthread_t));
    if (engines == NULL) free_everything("engines");
}

void free_everything(char* func) {
    free(rowToPrint);
    free(inputs);
    free(engines);
    free(col_mutex);
    free(column_sem);
    free(engine_sem);
    free(cols);

    if (strcmp(func, "nothing")) {
        printf("Error allocating memory for %s\n", func);
        exit(1);
    }
}

void destroy_everything() {
    sem_destroy(&writer_sem);
    pthread_mutex_destroy(&pixel_mutex);
    for (int i = 0; i < engine_count; i++) {
        sem_destroy(&engine_sem[i]);
    }
    for (int i = 0; i < engine_count; i++) {
        pthread_mutex_destroy(&col_mutex[i]);
    }
    // pthread_barrier_destroy(&col_and_write);
}