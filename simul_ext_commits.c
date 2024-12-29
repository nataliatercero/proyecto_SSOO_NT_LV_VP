#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100
#define MAX_BLOQUES 25

// Declaración de funciones
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

// Función para separar un comando en sus componentes
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    return sscanf(strcomando, "%s %s %s", orden, argumento1, argumento2);
}

// Imprime el mapa de bits de inodos y bloques
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    printf("\nInodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\nBloques:\n");
    for (int i = 0; i < MAX_BLOQUES; i++) {
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");
}

// Lee e imprime información básica del superbloque
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    printf("Superbloque:\n");
    printf("  Bloque: %u bytes\n", psup->s_block_size);
    printf("  Inodos particion: %u\n", psup->s_inodes_count);
    printf("  Inodos libres: %u\n", psup->s_free_inodes_count);
    printf("  Bloques particion: %u\n", psup->s_blocks_count);
    printf("  Bloques libres: %u\n", psup->s_free_blocks_count);
    printf("  Primer bloque de datos: %u\n", psup->s_first_data_block);
}

// Imprime el contenido del directorio
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    printf("Directorio:\n");
    for (int i = 1; i < MAX_FICHEROS; i++) { // Empieza en 1 para evitar la entrada raíz
        if (directorio[i].dir_inodo != NULL_INODO) {
            // Índice del inodo asociado
            int inodo_index = directorio[i].dir_inodo;

            // Validación del índice del inodo
            if (inodo_index < 0 || inodo_index >= MAX_INODOS) {
                printf("Error: Indice de inodo invalido para %s\n", directorio[i].dir_nfich);
                continue;
            }

            // Acceso al inodo
            EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[inodo_index];

            // Imprime información básica del archivo
            printf("%s\t\ttamanyo:%d\tinodo:%d\tbloques:", directorio[i].dir_nfich, inodo->size_fichero, inodo_index);

            // Recorre e imprime los bloques asociados
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodo->i_nbloque[j] != NULL_BLOQUE) {
                    printf("%d ", inodo->i_nbloque[j]);
                }
            }
            printf("\n");
        }
    }
}

// Guarda el contenido del directorio e inodos en el archivo de sistema
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    // Posiciona el puntero del archivo en la posición correspondiente a los directorios
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(directorio, SIZE_BLOQUE, 1, fich); // Guarda el directorio

    // Posiciona el puntero del archivo en la posición correspondiente a los inodos
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);
    fwrite(inodos, SIZE_BLOQUE, 1, fich); // Guarda los inodos
}

// Guarda el mapa de bits de inodos y bloques en el archivo de sistema
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    // Posiciona el puntero en la sección de mapas de bits
    fseek(fich, SIZE_BLOQUE * 1, SEEK_SET);
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich); // Guarda el mapa de bits
}

// Guarda el contenido del superbloque en el archivo de sistema
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    // Posiciona el puntero al inicio del archivo para escribir el superbloque
    fseek(fich, 0, SEEK_SET);
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich); // Guarda el superbloque
}

// Renombra un archivo en el directorio
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        // Busca el archivo con el nombre antiguo
        if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
            // Verifica que el nuevo nombre no exista ya
            for (int j = 0; j < MAX_FICHEROS; j++) {
                if (strcmp(directorio[j].dir_nfich, nombrenuevo) == 0) {
                    return -1; 
                }
            }
            // Renombra el archivo
            strcpy(directorio[i].dir_nfich, nombrenuevo);
            return 0; 
        }
    }
    return -1; 
}

// Imprime el contenido de un archivo dado su nombre
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    // Busca el archivo en el directorio
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) { // Archivo encontrado
            int inodo_index = directorio[i].dir_inodo;

            // Validar el índice del inodo
            if (inodo_index < 0 || inodo_index >= MAX_INODOS) {
                printf("Error: Indice de inodo invalido para el archivo %s\n", nombre);
                return -1;
            }

            EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[inodo_index];

            // Imprime el contenido almacenado en los bloques del archivo
            printf("Contenido de %s:\n", nombre);
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodo->i_nbloque[j] == NULL_BLOQUE) {
                    break; 
                }
                printf("%s", memdatos[inodo->i_nbloque[j]].dato); // Imprime contenido del bloque
            }
            printf("\n");
            return 0; 
        }
    }

    printf("Error: Archivo %s no encontrado\n", nombre);
    return -1; 
}
