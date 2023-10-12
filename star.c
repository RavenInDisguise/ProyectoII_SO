#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "verbose.h"

#define MAX_FILE_NAME 200 //TUVE QUE SUBIRLE AL LIMITE PARA HACER PRUEBAS CON NUESTROS FILES
#define MAX_ARCHIVE_SIZE 100

// Estructura para el encabezado de archivo en el archivo de salida
struct FileHeader {
    char name[MAX_FILE_NAME];
    unsigned int size;
};

void create_tar(const char *outputFile, int numFiles, char *inputFiles[]) {
    verbose("Nombre del archivo de salida: %s\n", outputFile);

    FILE *outFile = fopen(outputFile, "wb");
    if (!outFile) {
        perror("Error al abrir el archivo de salida\n");
        exit(1);
    }

    verbose("Abriendo los archivos de entrada...\n");

    for (int i = 0; i < numFiles; i++) {

        verbose("Leyendo %s en memoria...\n", inputFiles[i]);
        FILE *inputFile = fopen(inputFiles[i], "rb");
        if (!inputFile) {
            perror("Error al abrir el archivo de entrada\n");
            fclose(outFile);
            exit(1);
        }

        verbose("Calculando tamaño de archivo en paquete...\n");
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        rewind(inputFile);


        verbose("Creando cabecera de metadatos...\n");
        struct FileHeader header;
        strncpy(header.name, inputFiles[i], sizeof(header.name));
        header.size = (unsigned int)fileSize;
        fwrite(&header, sizeof(struct FileHeader), 1, outFile);


        verbose("Copiando datos al paquete %s...\n", outputFile);
        char *buffer;
        buffer = (char *)calloc(fileSize, sizeof(char));
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, fileSize, inputFile)) > 0) {
            fwrite(buffer, 1, bytesRead, outFile);
        }

        verbose("Liberando buffers de memoria...\n");
        free(buffer);
        fclose(inputFile);
        verbose("Se agregó el archivo %s al paquete %s\n", inputFiles[i], outputFile);
    }

    fclose(outFile);
    verbose("Se creó el archivo star: %s\n", outputFile);
}

void extract_tar(const char *archiveFile, int numFiles, char *filesToExtract[]) {
    //Implementa logica de extraer

    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    for (int i = 0; i < numFiles; i++) {
        bool found = false;
        struct FileHeader header;

        while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
            if (strcmp(header.name, filesToExtract[i]) == 0) {
                printf("Extrayendo: %s\n", header.name);
                FILE *outputFile = fopen(header.name, "wb");
                if (!outputFile) {
                    perror("Error al crear el archivo de salida");
                    fclose(tarFile);
                    exit(1);
                }

                char *buffer;
                buffer = (char *)calloc(header.size, sizeof(char));
                size_t bytesRead;

                while (header.size > 0) {
                    size_t readSize = (header.size > sizeof(buffer)) ? sizeof(buffer) : header.size;
                    bytesRead = fread(buffer, 1, readSize, tarFile);
                    fwrite(buffer, 1, bytesRead, outputFile);
                    header.size -= bytesRead;
                }

                free(buffer);
                fclose(outputFile);
                found = true;
                break;
            } else {
                // Si no es el archivo a extraer, avanzamos el puntero
                fseek(tarFile, header.size, SEEK_CUR);
            }
        }

        if (!found) {
            printf("Archivo no encontrado: %s\n", filesToExtract[i]);
        }
    }

    fclose(tarFile);
    printf("Extraer archivos de %s\n", archiveFile);
}

void list_tar(const char *archiveFile) {
    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    struct FileHeader header;
    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
        printf("Nombre de archivo: %s\n", header.name);
        printf("Tamaño del archivo: %u bytes\n", header.size);

        fseek(tarFile, header.size, SEEK_CUR);
    }

    fclose(tarFile);
    printf("Listar contenido de %s\n", archiveFile);
}

void delete_tar(const char *archiveFile, int numFiles, char *filesToDelete[]) {
    printf("Borrar archivos de %s\n", archiveFile);
}

void update_tar(const char *archiveFile, int numFiles, char *filesToUpdate[]) {
    printf("Actualizar archivo tar: %s\n", archiveFile);
}

int main(int argc, char *argv[]) {
    int argIndex = 1;
    char *outputFile = NULL;
    int numFiles = 0;
    char *inputFiles[argc];
    bool create = false, extract = false, list = false, delete = false, update = false;

    if (argc < 3) {
        fprintf(stderr, "Comando: %s -c|-x|-t|-d|-u -f <archivoSalida> <archivo1> <archivo2> ... <archivoN>\n", argv[0]);
        exit(1);
    }

    while (argIndex < argc) {
        if (strcmp(argv[argIndex], "-c") == 0) {
            create = true;
            argIndex++;
        } else if (strcmp(argv[argIndex], "-x") == 0) {
            extract = true;
            argIndex++;
        } else if (strcmp(argv[argIndex], "-t") == 0) {
            list = true;
            argIndex++;
        } else if (strcmp(argv[argIndex], "-u") == 0) {
            update = true;
            argIndex++;
        } else if (strcmp(argv[argIndex], "-v") == 0) {
            setVerbose(true);
            argIndex++;
        } else if (strcmp(argv[argIndex], "-f") == 0) {
            argIndex++;
            if (argIndex < argc) {
                outputFile = argv[argIndex];
                argIndex++;
            } else {
                fprintf(stderr, "Error: Se esperaba un nombre de archivo después de -f\n");
                exit(1);
            }
        } else {
            inputFiles[numFiles++] = argv[argIndex];
            argIndex++;
        }
    }

    if (create) {
        create_tar(outputFile, numFiles, inputFiles);
    } else if (extract) {
        extract_tar(outputFile, numFiles, inputFiles);
    } else if (list) {
        list_tar(outputFile);
    } else if (delete) {
        delete_tar(outputFile, numFiles, inputFiles);
    } else if (update) {
        update_tar(outputFile, numFiles, inputFiles);
    }

    return 0;
}
