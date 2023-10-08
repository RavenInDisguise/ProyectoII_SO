#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Estructura para el encabezado de archivo en el archivo de salida
struct FileHeader {
    char name[100];
    unsigned int size;
};

void create_tar(const char *outputFile, int numFiles, char *inputFiles[]) {
    // Implementa la lógica para crear un archivo tar aquí
    
    // Implementa la lógica para crear un archivo tar aquí
  
    printf("Nombre del archivo de salida: %s\n", outputFile); // Agrega esta línea para depurar

    // Abre el archivo de salida en modo escritura (creándolo si no existe)
    FILE *outFile = fopen(outputFile, "wb");
    if (!outFile)
    {
        perror("Error al abrir el archivo de salida");
        exit(1);
    }

    // Itera sobre los archivos de entrada y empaquétalos en el archivo de salida
    for (int i = 0; i < numFiles; i++)
    {
        FILE *inputFile = fopen(inputFiles[i], "rb");
        if (!inputFile)
        {
            perror("Error al abrir el archivo de entrada");
            fclose(outFile);
            exit(1);
        }

        // Lee el contenido del archivo de entrada
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        fseek(inputFile, 0, SEEK_SET);

        // Crea un encabezado para el archivo
        struct FileHeader header;
        strncpy(header.name, inputFiles[i], sizeof(header.name));
        header.size = (unsigned int)fileSize;

        // Escribe el encabezado en el archivo de salida
        fwrite(&header, sizeof(struct FileHeader), 1, outFile);

        // Escribe el contenido del archivo en el archivo de salida
        char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), inputFile)) > 0)
        {
            fwrite(buffer, 1, bytesRead, outFile);
        }

        // Cierra el archivo de entrada
        fclose(inputFile);
    }

    // Cierra el archivo de salida
    fclose(outFile);
    printf("Crear archivo tar: %s\n", outputFile);
}

void extract_tar(const char *archiveFile, int numFiles, char *filesToExtract[]) {
    // Implementa la lógica para extraer archivos de un archivo tar aquí
    // ...
    printf("Extraer archivos de %s\n", archiveFile);
}

void list_tar(const char *archiveFile) {
    // Implementa la lógica para listar los contenidos de un archivo tar aquí
    // ...
    printf("Listar contenido de %s\n", archiveFile);
}

void delete_tar(const char *archiveFile, int numFiles, char *filesToDelete[]) {
    // Implementa la lógica para borrar archivos de un archivo tar aquí
    // ...
    printf("Borrar archivos de %s\n", archiveFile);
}

void update_tar(const char *archiveFile, int numFiles, char *filesToUpdate[]) {
    // Implementa la lógica para actualizar un archivo tar aquí
    // ...
    printf("Actualizar archivo tar: %s\n", archiveFile);
}

int main(int argc, char *argv[]) {
    int argIndex = 1; // Índice para recorrer los argumentos
    char *outputFile = NULL;
    int numFiles = 0;
    char *inputFiles[argc];
    bool create = false, extract = false, list = false, delete = false, update = false;

    if (argc < 2) {
        fprintf(stderr, "Comando: %s <-opcion1> <-opcion2> <-opcionN> <archivoSalida> <archivo1> <archivo2> ... <archivoN>\n", argv[0]);
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
        } else if (strcmp(argv[argIndex], "-f") == 0) {
            argIndex++;
            if (argIndex < argc) {
                outputFile = argv[argIndex];
                argIndex++;
            } else {
                fprintf(stderr, "Error: Se esperaba un nombre de archivo después de -f\n");
                exit(1);
            }
        } else if (strcmp(argv[argIndex], "-d") == 0) {
            delete = true;
            argIndex++;
            if (argIndex < argc) {
                outputFile = argv[argIndex];
                argIndex++;
            } else {
                fprintf(stderr, "Error: Se esperaba un nombre de archivo después de -d\n");
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
