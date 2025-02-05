#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_BASE_PATH "./"

int get_id(const char *base_path);
int set_size(int size, const char *base_path);
void set_matr_a(int matrA[][4], int size, const char *base_path);
void set_matr_b(int matrB[], int size, const char *base_path);
int set_control(int value, const char *base_path);
void get_matr_c(int matrC[], int size, const char *base_path);
void print_matr_c(int matrC[], int size);
void print_usage(const char *program_name);

int main(int argc, char *argv[]) {
    int size = 4;
    int matrA[4][4] = {
        {1, 2, 4, 0},
        {0, 5, 7, 1},
        {1, 0, 5, 3},
        {1, 1, 0, 4}
    };
    int matrB[4] = {3, 1, 2, 4};
    int matrC[4];
    const char *base_path = DEFAULT_BASE_PATH;

    // Gestione degli argomenti della riga di comando
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
            case 'p':
                base_path = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    int id = get_id(base_path);
    if (id != -1) {
        printf("ID: %d\n", id);
    } else {
        return -1;
    }

    if (set_size(size, base_path) != 0) return -1;
    set_matr_a(matrA, size, base_path);
    set_matr_b(matrB, size, base_path);
    if (set_control(0x5, base_path) != 0) return -1;
    get_matr_c(matrC, size, base_path);
    print_matr_c(matrC, size);

    return 0;
}

void print_usage(const char *program_name) {
    printf("Utilizzo: %s [-p percorso_base] [-h]\n", program_name);
    printf("Opzioni:\n");
    printf("  -p percorso_base : Specifica il percorso base dei file (default: corrente)\n");
    printf("  -h               : Mostra questo messaggio di aiuto\n");
}

int get_id(const char *base_path) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/id", base_path);
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file id");
        return -1;
    }
    char buffer[32];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Errore nella lettura del file id");
        fclose(file);
        return -1;
    }
    fclose(file);
    int id;
    if (sscanf(buffer, "0x%x", &id) != 1) {
        fprintf(stderr, "Formato del file id non valido\n");
        return -1;
    }
    return id;
}

int set_size(int size, const char *base_path) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/size", base_path);
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file size");
        return -1;
    }
    fprintf(file, "0x%x", size);
    fclose(file);
    return 0;
}

void set_matr_a(int matrA[][4], int size, const char *base_path) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/matrA", base_path);
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file matrA");
        return;
    }
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            fprintf(file, "0x%x", matrA[i][j]);
            if (j != size - 1)
                fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

void set_matr_b(int matrB[], int size, const char *base_path) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/matrB", base_path);
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file matrB");
        return;
    }
    for (int i = 0; i < size; ++i) {
        fprintf(file, "0x%x", matrB[i]);
        if (i != size - 1)
            fprintf(file, ",");
    }
    fclose(file);
}

int set_control(int value, const char *base_path) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/control", base_path);
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file control");
        return -1;
    }
    fprintf(file, "0x%x", value);
    fclose(file);
    return 0;
}

void get_matr_c(int matrC[], int size, const char *base_path) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/matrC", base_path);
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file matrC");
        return;
    }
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Errore nella lettura del file matrC");
        fclose(file);
        return;
    }
    fclose(file);

    // Parsing dei valori esadecimali
    char *ptr = strtok(buffer, ",\n");
    int i = 0;
    while (ptr != NULL && i < size) {
        if (sscanf(ptr, "0x%x", &matrC[i]) != 1) {
            fprintf(stderr, "Formato non valido in matrC\n");
            return;
        }
        ptr = strtok(NULL, ",\n");
        i++;
    }
}

void print_matr_c(int matrC[], int size) {
    printf("Risultato vettore C:\n");
    for (int i = 0; i < size; ++i) {
        printf("%d ", matrC[i]);
    }
    printf("\n");
}
