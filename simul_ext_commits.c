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
