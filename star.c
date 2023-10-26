#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "verbose.h"

#define MAX_FILE_NAME 200 // TUVE QUE SUBIRLE AL LIMITE PARA HACER PRUEBAS CON NUESTROS FILES
#define MAX_ARCHIVE_SIZE 100

// Estructura para el encabezado de archivo en el archivo de salida
struct FileHeader
{
    char name[MAX_FILE_NAME];
    unsigned int size;
    bool deleted; // Me permite conocer si este file fue eliminado; ELIMINADO = T
};

void create_tar(const char *outputFile, int numFiles, char *inputFiles[])
{
    verbose("Nombre del archivo de salida: %s\n", outputFile);

    FILE *outFile = fopen(outputFile, "wb");
    if (!outFile)
    {
        perror("Error al abrir el archivo de salida\n");
        exit(1);
    }

    vverbose("Abriendo los archivos de entrada...\n");

    for (int i = 0; i < numFiles; i++)
    {

        vverbose("Leyendo %s en memoria...\n", inputFiles[i]);
        FILE *inputFile = fopen(inputFiles[i], "rb");
        if (!inputFile)
        {
            perror("Error al abrir el archivo de entrada\n");
            fclose(outFile);
            exit(1);
        }

        vverbose("Calculando tamaño de archivo en paquete...\n");
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        rewind(inputFile);

        vverbose("Creando cabecera de metadatos...\n");
        struct FileHeader header;
        strncpy(header.name, inputFiles[i], sizeof(header.name));
        header.size = (unsigned int)fileSize;
        fwrite(&header, sizeof(struct FileHeader), 1, outFile);

        vverbose("Copiando datos al paquete %s...\n", outputFile);
        char *buffer;
        buffer = (char *)calloc(fileSize, sizeof(char));
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, fileSize, inputFile)) > 0)
        {
            fwrite(buffer, 1, bytesRead, outFile);
        }

        vverbose("Liberando buffers de memoria...\n");
        free(buffer);
        fclose(inputFile);
        vverbose("Se agregó el archivo %s al paquete %s\n", inputFiles[i], outputFile);
    }

    fclose(outFile);
    verbose("Se creó el archivo star: %s\n", outputFile);
}

void extract_tar(const char *archiveFile, int numFiles, char *filesToExtract[])
{
    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile)
    {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    if (numFiles == 0)
    {
        // No se especificaron archivos específicos para extraer,
        // por lo que extraeremos todos los archivos del .tar.
        verbose("Extrayendo todos los archivos del archivo tar: %s\n", archiveFile);
        struct FileHeader header;
        while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1)
        {
            if (!header.deleted)
            {
                vverbose("Extrayendo: %s\n", header.name);
                FILE *outputFile = fopen(header.name, "wb");
                if (!outputFile)
                {
                    perror("Error al crear el archivo de salida");
                    fclose(tarFile);
                    exit(1);
                }

                char *buffer;
                buffer = (char *)calloc(header.size, sizeof(char));
                size_t bytesRead;

                while (header.size > 0)
                {
                    size_t readSize = (header.size > sizeof(buffer)) ? sizeof(buffer) : header.size;
                    bytesRead = fread(buffer, 1, readSize, tarFile);
                    fwrite(buffer, 1, bytesRead, outputFile);
                    header.size -= bytesRead;
                }

                free(buffer);
                fclose(outputFile);
            }
        }
    }
    else
    {
        // Se especificaron archivos específicos para extraer.
        verbose("Extrayendo archivos especificados del archivo tar: %s\n", archiveFile);
        for (int i = 0; i < numFiles; i++)
        {
            bool found = false;
            struct FileHeader header;

            while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1)
            {
                if (!header.deleted)
                {
                    if (strcmp(header.name, filesToExtract[i]) == 0)
                    {
                        vverbose("Extrayendo: %s\n", header.name);
                        FILE *outputFile = fopen(header.name, "wb");
                        if (!outputFile)
                        {
                            perror("Error al crear el archivo de salida");
                            fclose(tarFile);
                            exit(1);
                        }

                        char *buffer;
                        buffer = (char *)calloc(header.size, sizeof(char));
                        size_t bytesRead;

                        while (header.size > 0)
                        {
                            size_t readSize = (header.size > sizeof(buffer)) ? sizeof(buffer) : header.size;
                            bytesRead = fread(buffer, 1, readSize, tarFile);
                            fwrite(buffer, 1, bytesRead, outputFile);
                            header.size -= bytesRead;
                        }

                        free(buffer);
                        fclose(outputFile);
                        found = true;
                        break;
                    }
                    else
                    {
                        fseek(tarFile, header.size, SEEK_CUR);
                    }
                }
            }

            if (!found)
            {
                printf("Archivo no encontrado: %s\n", filesToExtract[i]);
            }
        }
    }

    fclose(tarFile);
    verbose("Extracción completada del archivo tar: %s\n", archiveFile);
}

void list_tar(const char *archiveFile)
{
    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile)
    {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    struct FileHeader header;
    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1)
    {
        if (!header.deleted)
        {
            printf("Nombre de archivo: %s\n", header.name);
            verbose("Tamaño del archivo: %u bytes\n", header.size);
        }
        else
        {

            vverbose("Tamaño del espacio libre: %u bytes\n", header.size);
        }
        fseek(tarFile, header.size, SEEK_CUR);
    }

    fclose(tarFile);
    printf("Listado de contenido completado de %s\n", archiveFile);
}

void delete_tar(const char *archiveFile, int numFiles, char *filesToDelete[])
{
    FILE *tarFile = fopen(archiveFile, "rb+");
    if (!tarFile)
    {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    verbose("Borrando archivos de %s\n", archiveFile);

    struct FileHeader header;
    long fileOffset = 0;

    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1)
    {
        if (!header.deleted)
        {
            bool found = false;
            for (int i = 0; i < numFiles; i++)
            {
                if (strcmp(header.name, filesToDelete[i]) == 0)
                {
                    vverbose("Borrando: %s\n", header.name);
                    header.deleted = true;
                    fseek(tarFile, fileOffset, SEEK_SET);
                    fwrite(&header, sizeof(struct FileHeader), 1, tarFile);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                fileOffset = ftell(tarFile);
                fileOffset += header.size;
                fseek(tarFile, fileOffset, SEEK_SET);
            }
        }
        else
        {
            // Salta los archivos marcados como borrados
            fileOffset = ftell(tarFile);
            fileOffset += header.size;
            fseek(tarFile, fileOffset, SEEK_SET);
        }
    }

    fclose(tarFile);
    verbose("Borrado completado en el archivo tar: %s\n", archiveFile);
}

void update_tar(const char *archiveFile, int numFiles, char *filesToUpdate[])
{
    verbose("Actualizar archivo tar: %s\n", archiveFile);

    FILE *outFile = fopen(archiveFile, "wb");
    if (!outFile)
    {
        perror("Error al abrir el archivo star\n");
        fclose(outFile);
        exit(1);
    }

    for (int i = 0; i < numFiles; i++)
    {
        verbose("Leyendo %s en memoria...", filesToUpdate[i]);
        FILE *inputFile = fopen(filesToUpdate[i], "rb");
        if (!inputFile)
        {
            perror("Error al abrir el archivo de entrada\n");
            fclose(outFile);
            exit(1);
        }

    }

}

/*
 * TODO: implementar logica de buscar hueco donde quepa el archivo nuevo
 */
void append_tar(const char *archiveFile, int numFiles, char *filesToAppend[])
{
    verbose("Añadiendo archivos al paquete %s\n", archiveFile);

    FILE *outFile = fopen(archiveFile, "ab");
    if (!outFile)
    {
        perror("Error al abrir el archivo de salida\n");
        fclose(outFile);
        exit(1);
    }

    for (int i = 0; i < numFiles; i++)
    {

        verbose("Leyendo %s en memoria...\n", filesToAppend[i]);
        FILE *inputFile = fopen(filesToAppend[i], "rb");
        if (!inputFile)
        {
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
        strncpy(header.name, filesToAppend[i], sizeof(header.name));
        header.size = (unsigned int)fileSize;
        fwrite(&header, sizeof(struct FileHeader), 1, outFile);

        verbose("Copiando datos al paquete %s...\n", archiveFile);
        char *buffer;
        buffer = (char *)calloc(fileSize, sizeof(char));
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, fileSize, inputFile)) > 0)
        {
            fwrite(buffer, 1, bytesRead, outFile);
        }

        verbose("Liberando buffers de memoria...\n");
        free(buffer);
        fclose(inputFile);
        verbose("Se agregó el archivo %s al paquete %s\n", filesToAppend[i], archiveFile);
    }

    fclose(outFile);
    verbose("Se agregó al paquete star: %s\n", archiveFile);
}

void defragment_tar(const char *archiveFile)
{
    verbose("Desfragmentando el archivo tar: %s\n", archiveFile);

    FILE *tarFile = fopen(archiveFile, "rb+");
    if (!tarFile)
    {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    struct FileHeader header;
    char buffer[MAX_ARCHIVE_SIZE];
    long newPosition = 0; // Nueva posición del archivo

    // Lista para almacenar información sobre los archivos que no se eliminarán
    struct FileHeader *validHeaders = NULL;
    int validHeaderCount = 0;

    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1)
    {
        if (!header.deleted)
        {
            // Copia el encabezado a la nueva posición
            fseek(tarFile, newPosition, SEEK_SET);
            fwrite(&header, sizeof(struct FileHeader), 1, tarFile);

            // Copia el contenido a la nueva posición

            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // EL COPIADO ES LO QUE CREO QUE FALLA A NIVEL LOGICO!!!!!
            // El cambio de encabezado si lo hace bien
            // De hecho curiosamente VS Code permite visualizar el contenido del tar

            size_t bytesRead;
            while (header.size > 0)
            {
                size_t readSize = (header.size > sizeof(buffer)) ? sizeof(buffer) : header.size;
                bytesRead = fread(buffer, 1, readSize, tarFile);
                printf("readSize: %zu\n", readSize); // Debug
                printf("bytesRead: %zu\n", bytesRead); // Debug
                if (bytesRead <= 0){
                    break; // Salir si no se leyeron bytes
                }

                fwrite(buffer, 1, bytesRead, tarFile);
                header.size -= bytesRead;
            }

            // Almacena el encabezado en la lista de archivos válidos
            validHeaders = (struct FileHeader *)realloc(validHeaders, (validHeaderCount + 1) * sizeof(struct FileHeader));
            validHeaders[validHeaderCount] = header;
            validHeaderCount++;

            // Calcula la nueva posición para el siguiente archivo
            newPosition = ftell(tarFile);
        }
        else
        {
            // Salta archivos marcados como borrados
            fseek(tarFile, header.size, SEEK_CUR);
        }
    }

    // Trunca el archivo al nuevo tamaño
    int result = ftruncate(fileno(tarFile), newPosition);
    if (result != 0)
    {
        perror("Error al truncar el archivo");
        fclose(tarFile);
        exit(1);
    }

    fclose(tarFile);
    verbose("Desfragmentación completada en el archivo tar: %s\n", archiveFile);

    // Liberar la memoria asignada a validHeaders 
    free(validHeaders);
}

int main(int argc, char *argv[])
{
    int argIndex = 1;
    char *outputFile = NULL;
    int numFiles = 0;
    char *inputFiles[argc];
    int commandSize;
    bool create = false, extract = false, list = false, delete = false, update = false, append = false, defragment = false;
    bool usedFile = false;

    if (argc < 3)
    {
        fprintf(stderr, "Comando: %s -c|x|t|d|u|v|vv|f <archivoSalida> <archivo1> <archivo2> ... <archivoN>\n", argv[0]);
        exit(1);
    }

    char* commandStr = argv[1];
    commandSize = strlen(commandStr);

    int i = 1; // inicia en 1 porque se asume el comando inicia con el "-"

    while (i < commandSize)
    {
        if (commandStr[i] == 'c')
        {
            create = true;
        }
        else if (commandStr[i] == 'x')
        {
            extract = true;
        }
        else if (commandStr[i] == 't')
        {
            list = true;
        }
        else if (commandStr[i] == 'r')
        {
            append = true;
        }
        else if (commandStr[i] == 'u')
        {
            update = true;
        }
        else if (commandStr[i] == 'd')
        {
            delete = true;
        }
        else if (commandStr[i] == 'p')
        {
            defragment = true;
        }
        else if (commandStr[i] == 'v')
        {
            if (i + 1 < commandSize)
            {
                if (commandStr[i+1] == 'v')
                {
                    setStrongVerbose(true);
                }
            }
            setVerbose(true);
        }
        else if (commandStr[i] == 'f')
        {
            // Aqui se supone es que significa que le estoy dando el archivo star
            // Si los comandos ahora son siempre en un solo argumento, el outputfile siempre sera el argumento 2
            outputFile = argv[2];
            usedFile = true;
        }
        i++;
    }

    if (!usedFile)
    {
        fprintf(stderr, "Error: Se debe especificar un nombre de archivo después del comando -f\n");
        exit(1);
    }

    // recorre el resto de argumentos para los inputFiles
    for (int i = 3; i < argc; i++)
    {
        inputFiles[numFiles++] = argv[i];
    }
/*
    while (argIndex < argc)
    {
        if (strcmp(argv[argIndex], "-c") == 0)
        {
            create = true;
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-x") == 0)
        {
            extract = true;
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-t") == 0)
        {
            list = true;
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-u") == 0)
        {
            update = true;
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-vv") == 0)
        {
            setStrongVerbose(true);
            setVerbose(true);
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-v") == 0)
        {
            setVerbose(true);
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-r") == 0)
        {
            append = true;
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-p") == 0)
        {
            defragment = true;
            argIndex++;
        }
        else if (strcmp(argv[argIndex], "-f") == 0)
        {
            argIndex++;
            if (argIndex < argc)
            {
                outputFile = argv[argIndex];
                argIndex++;
            }
            else
            {
                fprintf(stderr, "Error: Se esperaba un nombre de archivo después de -f\n");
                exit(1);
            }
        }
        else if (strcmp(argv[argIndex], "-d") == 0)
        {
            delete = true;
            argIndex++;
        }
        else
        {
            inputFiles[numFiles++] = argv[argIndex];
            argIndex++;
        }
    }
*/
    if (create)
    {
        create_tar(outputFile, numFiles, inputFiles);
    }
    else if (extract)
    {
        extract_tar(outputFile, numFiles, inputFiles);
    }
    else if (list)
    {
        list_tar(outputFile);
    }
    else if (delete)
    {
        delete_tar(outputFile, numFiles, inputFiles);
    }
    else if (update)
    {
        update_tar(outputFile, numFiles, inputFiles);
    }
    else if (append)
    {
        append_tar(outputFile, numFiles, inputFiles);
    }
    else if (defragment)
    {
        defragment_tar(outputFile);
    }

    return 0;
}
