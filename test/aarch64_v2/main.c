#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>

// IOCTL command definitions for communication with the device
#define RD_ID               _IOR('a','b',int32_t*)
#define RD_STATUS           _IOW('a','c',int32_t*)

#define CTRL_ENABLE_DEV     _IOR('a','d',int32_t*)
#define CTRL_DISABLE_DEV    _IOR('a','e',int32_t*)
#define CTRL_ENABLE_IRQ     _IOR('a','f',int32_t*)
#define CTRL_DISABLE_IRQ    _IOR('a','g',int32_t*)
#define CTRL_START_OP       _IOR('a','h',int32_t*)
#define CTRL_RESET_STAT     _IOR('a','i',int32_t*)

#define RD_SIZE             _IOR('a','j',int32_t*)
#define WR_SIZE             _IOR('a','k',int32_t*)
#define RD_MATRA            _IOR('a','l',int32_t*)
#define WR_MATRA            _IOR('a','m',int32_t*)
#define RD_MATRB            _IOR('a','n',int32_t*)
#define WR_MATRB            _IOR('a','o',int32_t*)
#define RD_MATRC            _IOR('a','p',int32_t*)

void print_usage(const char *prog_name) {
    printf("Usage: %s [-p device_path] [-s size_matrices] [-h]\n", prog_name);
    printf("  -p device_file_path   : Specify the path to the device file (default: /dev/mulmatr_core)\n");
    printf("  -s size matrix        : Set matrices size (default: 4)\n");
    printf("  -a mat_a_file         : Set matrix A file path (default: /root/matrix_a.txt)\n");
    printf("  -b mat_b_file         : Set matrix B file path (default: /root/matrix_b.txt)\n");
    printf("  -h                    : Show this help message\n");
}

int read_matrix_from_file(const char *filename, int32_t *mat, int rows, int cols) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening matrix file");
        return -1;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(file, "%d", &mat[i * cols + j]) != 1) {
                fclose(file);
                perror("Error matrix data reading from file\n");
                return -1;
            }
        }
    }
    
    fclose(file);
    return 0;
}

void print_matrix(int32_t *mat, int rows, int cols){

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", mat[i * cols + j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {

    int opt;

    char *file_path_mat_a = "/root/matrix_a.txt";
    char *file_path_mat_b = "/root/matrix_b.txt";

    char *file_path = "/dev/mulmatr_core"; // default device path

    int size_mat = 4; // default matrices size

    int fd;

    int32_t id, status;
    int ret_size_mat = -1;
    
    
    // Parsing command line arguments
    while ((opt = getopt(argc, argv, "p:s:h")) != -1) {
        switch (opt) {
            case 'p':
                file_path = optarg;
                break;
            case 's':
                size_mat = atoi(optarg);
                break;
            case 'a':
                file_path_mat_a = optarg;
                break;
            case 'b':
                file_path_mat_b = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Dynamic allocation of matrices
    int32_t *mat_a = (int32_t*)malloc(size_mat * size_mat * sizeof(int32_t));
    int32_t *mat_b = (int32_t*)malloc(size_mat * sizeof(int32_t));

    // Matrices initialization
    memset(mat_a, 0, size_mat * size_mat * sizeof(int32_t));
    memset(mat_b, 0, size_mat * sizeof(int32_t));

    // Dynamic allocation of matrices read from device
    int32_t *ret_mat_a = (int32_t*)malloc(size_mat * size_mat * sizeof(int32_t));
    int32_t *ret_mat_b = (int32_t*)malloc(size_mat * sizeof(int32_t));
    int32_t *ret_mat_c = (int32_t*)malloc(size_mat * sizeof(int32_t));

    // Matrices initialization
    memset(ret_mat_a, 0, size_mat * size_mat * sizeof(int32_t));
    memset(ret_mat_b, 0, size_mat * sizeof(int32_t));
    memset(ret_mat_c, 0, size_mat * sizeof(int32_t));

    // Set matrix A
    if (read_matrix_from_file(file_path_mat_a, mat_a, size_mat, size_mat) < 0){
        free(mat_a);
        free(mat_b);
        free(ret_mat_a);
        free(ret_mat_b);
        free(ret_mat_c);
        return EXIT_FAILURE;
    }
    printf("Matrix A loaded from file:\n");
    print_matrix(mat_a, size_mat, size_mat);

    // Set matrix B
    if (read_matrix_from_file(file_path_mat_b, mat_b, size_mat, 1) < 0){
        free(mat_a);
        free(mat_b);
        free(ret_mat_a);
        free(ret_mat_b);
        free(ret_mat_c);
        return EXIT_FAILURE;
    }
    printf("Matrix B loaded from file:\n");
    print_matrix(mat_b, size_mat, 1);

    // Open device file
    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("Error opening device file\n");
        return EXIT_FAILURE;
    }

    // Read device ID
    if (ioctl(fd, RD_ID, &id) < 0) {
        perror("Error calling ioctl RD_ID\n");
        return EXIT_FAILURE;
    }
    printf("ID device: %d\n", id);

    // Enable device
    if (ioctl(fd, CTRL_ENABLE_DEV, NULL) < 0) {
        perror("Error calling ioctl CTRL_ENABLE_DEV\n");
        return EXIT_FAILURE;
    }
    printf("Device enabled\n");

    // Set matrices size
    if (ioctl(fd, WR_SIZE, &size_mat) < 0) {
        perror("Error calling ioctl WR_SIZE\n");
        return EXIT_FAILURE;
    }
    printf("Matrix size set to %d\n", size_mat);

    // Set matrix A
    if (ioctl(fd, WR_MATRA, mat_a) < 0) {
        perror("Errore nella chiamata ioctl WR_MATRA\n");
        return EXIT_FAILURE;
    }
    printf("Matrix A set\n");

    // Set matrix B
    if (ioctl(fd, WR_MATRB, mat_b) < 0) {
        perror("Errore nella chiamata ioctl WR_MATRB\n");
        return EXIT_FAILURE;
    }
    printf("Matrix B set\n");

    // Read matrix A
    if (ioctl(fd, RD_MATRA, ret_mat_a) < 0) {
        perror("Error calling ioctl RD_MATRA\n");
        return EXIT_FAILURE;
    }
    printf("Matrix A read:\n");
    print_matrix(ret_mat_a, size_mat, size_mat);

    // Read matrix B
    if (ioctl(fd, RD_MATRB, ret_mat_b) < 0) {
        perror("Error calling ioctl RD_MATRB\n");
        return EXIT_FAILURE;
    }
    printf("Matrix B read:\n");
    print_matrix(ret_mat_b, size_mat, 1);

    // Read status
    if (ioctl(fd, RD_STATUS, &status) < 0) {
        perror("Error calling RD_STATUS\n");
        return EXIT_FAILURE;
    }
    printf("Status read before start: %d\n", status);

    // Start operation
    if (ioctl(fd, CTRL_START_OP, NULL) < 0) {
        perror("Error calling ioctl CTRL_START_OP");
        return EXIT_FAILURE;
    }
    printf("Operation started\n");

    // Read status after start
    if (ioctl(fd, RD_STATUS, &status) < 0) {
        perror("Error calling ioctl RD_STATUS");
        return EXIT_FAILURE;
    }
    printf("Status read after start: %d\n", status);

    // Read result vector
    if (ioctl(fd, RD_MATRC, ret_mat_c) < 0) {
        perror("Error calling ioctl RD_MATRC");
        return EXIT_FAILURE;
    }
    printf("Matrix C: ");
    print_matrix(ret_mat_c, 1, size_mat);

    // Read status before reset
    if (ioctl(fd, RD_STATUS, &status) < 0) {
        perror("Error calling ioctl RD_STATUS");
        return EXIT_FAILURE;
    }
    printf("Status read before reset: %d\n", status);

    // Reset status
    if (ioctl(fd, CTRL_RESET_STAT, NULL) < 0) {
        perror("Error calling ioctl CTRL_RESET_STAT");
        return EXIT_FAILURE;
    }
    printf("Status resetted\n");

    // Read status after reset
    if (ioctl(fd, RD_STATUS, &status) < 0) {
        perror("Error calling ioctl RD_STATUS");
        return EXIT_FAILURE;
    }
    printf("Status read before reset: %d\n", status);

    // close file
    close(fd);
    printf("File closed correctly\n");

    return EXIT_SUCCESS;
}
