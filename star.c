#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "verbose.h"

#define MAX_FILE_NAME 200 // TUVE QUE SUBIRLE AL LIMITE PARA HACER PRUEBAS CON NUESTROS FILES
#define MAX_ARCHIVE_SIZE 100

// Estructura para el encabezado de archivo en el archivo de salida
typedef struct FileHeader
{
    char name[MAX_FILE_NAME];
    unsigned int size;
    unsigned int jump;
    bool deleted; // Me permite conocer si este file fue eliminado; ELIMINADO = T
} f_h;

const int HEADER_SIZE = sizeof(f_h);

// Busca bloques vacios en el paquete y devuelve la posicion del primer bloque vacio que pueda almacenar el archivo a buscar
// Si no hay bloques vacios, o no encuentra uno de suficiente tamaño devuelve -1
int check_file_in_block(FILE *tarFile, FILE *matchFile)
{
    int sizeToMatch, foundPosition;
    vverbose("Buscando bloques libres compatibles...\n");

    // Calcular el sizeToMatch
    fseek(matchFile, 0, SEEK_END);
    sizeToMatch = ftell(matchFile);
    rewind(matchFile);

    vverbose("Buscando bloques de al menos %u bytes\n");

    f_h header;
    while (fread(&header, HEADER_SIZE, 1, tarFile) == 1)
    {
        // si es marcado como deleted es un bloque vacio
        if (header.deleted)
        {
            vverbose("Bloque libre de %lu bytes...\n", header.size);
            if (sizeToMatch <= header.size)
            {
                vverbose("Bloque es de tamaño suficiente!\n");
                // Encuentra la posicion actual, menos el header, para que sobreescriba en caso de append
                vverbose("Posicion actual for some fucking reason: %ld", ftell(tarFile));
                foundPosition = (ftell(tarFile) - HEADER_SIZE);
                vverbose("foundPosition calculado (debe ser el ftell menos 268: %d)", foundPosition);
                rewind(tarFile);
                rewind(matchFile);
                return foundPosition;
            }
        }
    }
    rewind(tarFile);
    rewind(matchFile);
    return -1;
}

int fuse_empty_blocks(const char *tarFile)
{
    vverbose("Buscando bloques vacios contiguos...\n");
    FILE *tarPkg = fopen(tarFile, "r+");
    if (!tarPkg)
    {
        perror("Error al abrir el paquete star\n");
        exit(1);
    }



    f_h header;

    while (fread(&header, HEADER_SIZE, 1, tarPkg) == 1)
    {
        
    }

    return 0;
}

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
        header.jump = (unsigned int)fileSize;
        header.deleted = false;
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
void extract_tar(const char *archiveFile, int numFiles, char *filesToExtract[]) {
    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    if (numFiles == 0) {
        // No se especificaron archivos específicos para extraer,
        // por lo que extraeremos todos los archivos del .tar.
        verbose("Extrayendo todos los archivos del archivo tar: %s\n", archiveFile);
        struct FileHeader header;
        while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
            if (!header.deleted) {
                vverbose("Extrayendo: %s\n", header.name);
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
            } else {
                fseek(tarFile, header.size, SEEK_CUR); // Saltar archivos marcados como eliminados
            }
        }
    } else {
        // Se especificaron archivos específicos para extraer.
        verbose("Extrayendo archivos especificados del archivo tar: %s\n", archiveFile);
        for (int i = 0; i < numFiles; i++) {
            struct FileHeader header;
            bool found = false;

            while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
                if (!header.deleted) {
                    if (strcmp(header.name, filesToExtract[i]) == 0) {
                        vverbose("Extrayendo: %s\n", header.name);
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
                        fseek(tarFile, header.size, SEEK_CUR); // Saltar otros archivos
                    }
                } else {
                    fseek(tarFile, header.size, SEEK_CUR); // Saltar archivos marcados como eliminados
                }
            }

            if (!found) {
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
            printf("----------\n");
        }
        else
        {
            verbose("----Bloque vacio----\n");
            vverbose("Tamaño del espacio libre: %u bytes\n", header.size);
            verbose("----------\n");
        }
        fseek(tarFile, header.jump, SEEK_CUR);
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
                    verbose("Borrando: %s\n", header.name);
                    vverbose("Tamaño del archivo: %u bytes\n", header.jump);
                    header.deleted = true;
                    header.size = header.jump;
                    fseek(tarFile, fileOffset, SEEK_SET);
                    fwrite(&header, sizeof(struct FileHeader), 1, tarFile);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                fileOffset = ftell(tarFile);
                fileOffset += header.jump;
                fseek(tarFile, fileOffset, SEEK_SET);
            }
        }
        else
        {
            // Salta los archivos marcados como borrados
            fileOffset = ftell(tarFile);
            fileOffset += header.jump;
            fseek(tarFile, fileOffset, SEEK_SET);
        }
    }

    fclose(tarFile);
    verbose("Borrado completado en el archivo tar: %s\n", archiveFile);
}

void append_tar_without_check(const char *archiveFile, int numFiles, char *filesToAppend[])
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

        vverbose("Leyendo %s en memoria...\n", filesToAppend[i]);
        FILE *inputFile = fopen(filesToAppend[i], "rb");
        if (!inputFile)
        {
            perror("Error al abrir el archivo de entrada\n");
            fclose(inputFile);
            fclose(outFile);
            exit(1);
        }

        vverbose("Calculando tamaño de archivo en paquete...\n");
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        rewind(inputFile);

        vverbose("Creando cabecera de metadatos...\n");
        struct FileHeader header;
        strncpy(header.name, filesToAppend[i], sizeof(header.name));
        header.size = (unsigned int)fileSize;
        header.jump = (unsigned int)fileSize;
        header.deleted = false;
        fwrite(&header, sizeof(struct FileHeader), 1, outFile);

        vverbose("Copiando datos al paquete %s...\n", archiveFile);
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
        vverbose("Se agregó el archivo %s al paquete %s\n", filesToAppend[i], archiveFile);
    }

    fclose(outFile);
    verbose("Se agregó al paquete star: %s\n", archiveFile);
}

void append_tar_with_check(const char *archiveFile, int numFiles, char *filesToAppend[])
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
            fclose(inputFile);
            fclose(outFile);
            exit(1);
        }

        vverbose("Calculando tamaño de archivo en paquete...\n");
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        rewind(inputFile);

        // Buscar si cabe en un bloque vacio
        rewind(outFile);
        rewind(inputFile);
        int offset = check_file_in_block(outFile, inputFile);
        char *buffer;
        // Si no hay bloques disponibles para el update se agrega al final
        if (offset < 0)
        {
            vverbose("No se encontró bloques de suficiente tamaño, se va a agregar al final del paquete\n");
            fclose(outFile);
            fclose(inputFile);
            vverbose("Agregando entrada al final...\n");
            append_tar_without_check(archiveFile, 1, (char *[]){filesToAppend[i]});
            return;
        }
        // Caso contrario: agregar en el offset dado
        else
        {
            vverbose("Se encontró un espacio en el offset: %ld del paquete", offset);
            // Coloca el puntero al lugar donde se va a sobreescribir los datos (del bloque vacio)
            fseek(outFile, offset, SEEK_SET);
            rewind(inputFile);

            vverbose("Creando cabecera de metadatos...\n");
            f_h header;
            strncpy(header.name, filesToAppend[i], sizeof(header.name));
            header.size = (unsigned int)fileSize;
            header.jump = (unsigned int)fileSize;
            header.deleted = false;
            fwrite(&header, HEADER_SIZE, 1, outFile);

            vverbose("Copiando datos al paquete %s...\n", archiveFile);
            buffer = (char *)calloc(fileSize, sizeof(char));
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, fileSize, inputFile)) > 0)
            {
                fwrite(buffer, 1, bytesRead, outFile);
            }

            vverbose("Liberando buffers de memoria...\n");
            free(buffer);
            fclose(inputFile);
            vverbose("Se agregó el archivo %s al paquete %s\n", filesToAppend[i], archiveFile);
        }

        vverbose("Liberando buffers de memoria...\n");
        free(buffer);
        fclose(inputFile);
        verbose("Se agregó el archivo %s al paquete %s\n", filesToAppend[i], archiveFile);
    }

    fclose(outFile);
    verbose("Se agregó con éxito al paquete star: %s\n", archiveFile);
}

void update_tar(const char *archiveFile, int numFiles, char *filesToUpdate[])
{
    verbose("Actualizar archivo tar: %s\n", archiveFile);

    FILE *outFile = fopen(archiveFile, "r+");
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

        int newFileSize;
        fseek(inputFile, 0, SEEK_END);
        newFileSize = ftell(inputFile);
        rewind(inputFile);
        vverbose("Nuevo tamaño del archivo: %u bytes\n", newFileSize);

        // Primer check: si cabe en el bloque original
        f_h header;
        bool hasCrumbs = false;
        vverbose("Primer check: si cabe en el archivo anterior...\n");
        while (fread(&header, HEADER_SIZE, 1, outFile) == 1)
        {
            hasCrumbs = false;
            if (!(header.deleted) && (strcmp(header.name, filesToUpdate[i]) == 0))
            {
                vverbose("Encontro el archivo original, probando tamaño...\n");
                vverbose("Original: %lu bytes vs nuevo: %lu bytes\n", header.size, newFileSize);
                if (newFileSize <= header.size)
                {
                    vverbose("Archivo es menor al almacenado previamente...\n");
                    // Se sobreescriben los datos
                    // Calcular cuanto de espacio vacio queda de la sobreescritura
                    int crumbBytes = (int)header.size - newFileSize;
                    // Si el tamaño del sobrante es menor al de una cabecera quedará como bytes flotantes
                    if (crumbBytes < HEADER_SIZE)
                    {
                        hasCrumbs = true;
                    }
/*
                    if (!hasCrumbs)
                    {
                        // Solo modifica el tamaño en la cabecera si hay suficiente espacio para minimo otra
                        header.size = newFileSize;
                        header.jump = header.jump;
                    }
*/
                    header.size = (unsigned int)newFileSize;

                    // Regresa el puntero al inicio de la cabecera para sobreescribir
                    vverbose("Posicion actual del puntero: %lu\n", ftell(outFile));
                    fseek(outFile, -HEADER_SIZE, SEEK_CUR);
                    vverbose("Se supone que se mueve %lu bytes para atras: %lu\n", HEADER_SIZE, ftell(outFile));
                    fwrite(&header, HEADER_SIZE, 1, outFile);
                    vverbose("Aqui deberia volver al print de posicion actual: %lu\n", ftell(outFile));
                    char *buffer;
                    buffer = (char *)calloc(newFileSize, sizeof(char));
                    size_t bytesRead;
                    while ((bytesRead = fread(buffer, 1, newFileSize, inputFile)) > 0)
                    {
                        fwrite(buffer, 1, bytesRead, outFile);
                    }
                    vverbose("Aqui deberia moverse estar en %lu porque se movio %ld bytes de reescritura\n", ftell(outFile), newFileSize);


                    if (!hasCrumbs)
                    {
                        f_h emptyHeader;
                        strncpy(emptyHeader.name, "EMPTY", sizeof(emptyHeader.name));
                        emptyHeader.size = (unsigned int)(crumbBytes - HEADER_SIZE);
                        emptyHeader.jump = (unsigned int)(crumbBytes - HEADER_SIZE);
                        emptyHeader.deleted = true;
                        fwrite(&emptyHeader, HEADER_SIZE, 1, outFile);
                    }

                    // Finaliza ejecucion
                    free(buffer);
                    fclose(inputFile);
                    fclose(outFile);
                    return;
                }
            }
        }

        rewind(outFile);
        rewind(inputFile);

        // Buscar si cabe en un bloque vacio
        vverbose("No se encontró el original, buscando bloques vacios...\n");
        int offset = check_file_in_block(outFile, inputFile);

        // Si no hay bloques disponibles para el update se marca el bloque anterior como deleted y se hace append
        if (offset < 0)
        {
            vverbose("No se encontró bloques de suficiente tamaño, se va a agregar al final del paquete\n");
            fclose(outFile);
            fclose(inputFile);
            vverbose("Borrando entrada anterior...\n");
            delete_tar(archiveFile, 1, (char *[]){filesToUpdate[i]});
            vverbose("Agregando entrada al final...\n");
            append_tar_without_check(archiveFile, 1, (char *[]){filesToUpdate[i]});
            return;
        }
        // Caso contrario: actualizar en el offset dado
        else
        {
            // Coloca el puntero al lugar donde se va a sobreescribir los datos
            vverbose("Encontro un bloque en posicion %lu, actualmente en %lu\n", offset, ftell(outFile));
            fseek(outFile, offset, SEEK_SET);
            rewind(inputFile);
            vverbose("Ahora %lu(off) y %lu(pos) deberian ser iguales\n", offset, ftell(outFile));
            f_h updateHeader;
            strncpy(updateHeader.name, filesToUpdate[i], sizeof(updateHeader.name));
            updateHeader.size = (unsigned int)newFileSize;
            updateHeader.jump = (unsigned int)0;
            updateHeader.deleted = false;
            vverbose("Escribiendo %ld bytes (cabecera) actual: %lu\n", HEADER_SIZE, ftell(outFile));
            fwrite(&header, HEADER_SIZE, 1, outFile);
            vverbose("Escritura de cabecera hecha, actual: %lu\n", ftell(outFile));
            vverbose("Ahora escribir %ld bytes, actual: %lu\n", newFileSize, ftell(outFile));
            fwrite(&inputFile, newFileSize, 1, outFile);
            vverbose("Escritura hecha, nueva posicion: %lu\n",ftell(outFile));
            fclose(inputFile);
            fclose(outFile);
            verbose("Update done\n");
            return;
        }
    }
}

void defragment_tar(const char *archiveFile) {
    verbose("Desfragmentación en curso en el archivo tar: %s\n", archiveFile);

    FILE *tarFile = fopen(archiveFile, "r+");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    struct FileHeader header;
    long currentOffset = 0;
    long newOffset = 0;

    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
        vverbose("Haciendo copia de los archivos...\n");
        if (header.deleted) {
            vverbose("No se hace copia de los archivos marcados como eliminado.\n");
            fseek(tarFile, header.size, SEEK_CUR);
        } else {
            if (currentOffset != newOffset) {
                // Si no están en la misma posición, debemos mover el archivo a la nueva posición (newOffset)
                // y actualizar el encabezado.
                verbose("Se copia el contenido de los archivos que no fueron eliminados.\n");
                fseek(tarFile, currentOffset, SEEK_SET);
                char *buffer;
                buffer = (char *)calloc(header.size, sizeof(char));
                size_t bytesRead = 0;

                while (bytesRead < header.size) {
                    size_t readSize = (header.size - bytesRead > sizeof(buffer)) ? sizeof(buffer) : (header.size - bytesRead);
                    size_t read = fread(buffer, 1, readSize, tarFile);
                    if (read > 0) {
                        fwrite(buffer, 1, read, tarFile);
                        bytesRead += read;
                    }
                }

                vverbose("Actualiza el encabezado en la nueva posición \n");
                fseek(tarFile, newOffset, SEEK_SET);
                fwrite(&header, sizeof(struct FileHeader), 1, tarFile);

                // Coloca el puntero al final del nuevo bloque.
                fseek(tarFile, newOffset + header.size, SEEK_SET);
                free(buffer);
            }
            newOffset = ftell(tarFile); // Guarda la nueva posición.
            currentOffset = newOffset + header.size; // Calcula la posición actual.
        }
    }

    vverbose("Trunca el archivo para eliminar el contenido restante de los archivos eliminados.\n");
    ftruncate(fileno(tarFile), ftell(tarFile));
    fclose(tarFile);

    verbose("Desfragmentación completada en el archivo tar: %s\n", archiveFile);
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

    char *commandStr = argv[1];
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
                if (commandStr[i + 1] == 'v')
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
        // TODO cambiar nombre de funcion
        append_tar_with_check(outputFile, numFiles, inputFiles);
    }
    else if (defragment)
    {
        defragment_tar(outputFile);
    }

    return 0;
}
