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

// Borra un archivo del directorio y libera sus recursos
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque, char *nombre, FILE *fich) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) { 
            int inodo_index = directorio[i].dir_inodo;

            // Libera los bloques asociados al archivo
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodos->blq_inodos[inodo_index].i_nbloque[j] == NULL_BLOQUE) {
                    break;
                }
                bytemaps->bmap_bloques[inodos->blq_inodos[inodo_index].i_nbloque[j]] = 0; // Bloque libre
                inodos->blq_inodos[inodo_index].i_nbloque[j] = NULL_BLOQUE; // Bloque anulado
            }

            // Libera el inodo asociado al archivo
            inodos->blq_inodos[inodo_index].size_fichero = 0;
            bytemaps->bmap_inodos[inodo_index] = 0;

            // Elimina la entrada del directorio
            strcpy(directorio[i].dir_nfich, "");
            directorio[i].dir_inodo = NULL_INODO;

            // Actualiza el superbloque
            superbloque->s_free_blocks_count++;
            superbloque->s_free_inodes_count++;

            // Guarda los cambios en el archivo de sistema
            Grabarinodosydirectorio(directorio, inodos, fich);
            GrabarByteMaps(bytemaps, fich);
            GrabarSuperBloque(superbloque, fich);

            return 0; 
        }
    }
    return -1; 
}

// Copia un archivo a una nueva entrada del directorio
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich) {
    int origen_index = -1, destino_index = -1;

    // Busca el archivo de origen y verifica que el destino no exista
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreorigen) == 0) {
            origen_index = directorio[i].dir_inodo;
        }
        if (strcmp(directorio[i].dir_nfich, nombredestino) == 0) {
            return -1; 
        }
    }

    if (origen_index == -1) {
        return -1; 
    }

    // Busca una entrada libre en el directorio para el archivo destino
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            destino_index = i;
            break;
        }
    }

    if (destino_index == -1) {
        return -1; 
    }

    // Busca un inodo libre para el archivo destino
    int nuevo_inodo = -1;
    for (int i = 0; i < MAX_INODOS; i++) {
        if (bytemaps->bmap_inodos[i] == 0) {
            nuevo_inodo = i;
            break;
        }
    }

    if (nuevo_inodo == -1) {
        return -1; 
    }

    // Marca el inodo como ocupado y crea la nueva entrada en el directorio
    bytemaps->bmap_inodos[nuevo_inodo] = 1;
    strcpy(directorio[destino_index].dir_nfich, nombredestino);
    directorio[destino_index].dir_inodo = nuevo_inodo;

    // Copia el tamaño y bloques del archivo
    inodos->blq_inodos[nuevo_inodo].size_fichero = inodos->blq_inodos[origen_index].size_fichero;
    for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        if (inodos->blq_inodos[origen_index].i_nbloque[j] == NULL_BLOQUE) {
            inodos->blq_inodos[nuevo_inodo].i_nbloque[j] = NULL_BLOQUE;
            break;
        }
        for (int k = 0; k < MAX_BLOQUES_PARTICION; k++) {
            if (bytemaps->bmap_bloques[k] == 0) {
                bytemaps->bmap_bloques[k] = 1;
                inodos->blq_inodos[nuevo_inodo].i_nbloque[j] = k;
                memcpy(memdatos[k].dato, memdatos[inodos->blq_inodos[origen_index].i_nbloque[j]].dato, SIZE_BLOQUE);
                break;
            }
        }
    }

    // Actualiza el archivo de sistema
    fseek(fich, 0, SEEK_SET);
    fwrite(superbloque, SIZE_BLOQUE, 1, fich);
    fseek(fich, SIZE_BLOQUE, SEEK_SET);
    fwrite(bytemaps, SIZE_BLOQUE, 1, fich);
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);
    fwrite(inodos, SIZE_BLOQUE, 1, fich);
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(directorio, SIZE_BLOQUE, 1, fich);

    return 0; 
}


// Función principal 
int main() {
    char comando[LONGITUD_COMANDO];
    char orden[LONGITUD_COMANDO];
    char argumento1[LONGITUD_COMANDO];
    char argumento2[LONGITUD_COMANDO];

    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];

    // Abre el archivo 
    FILE *fent = fopen("particion.bin", "r+b");
    if (!fent) {
        perror("Error al abrir particion.bin");
        return 1;
    }

    // Carga estructuras
    fread(&ext_superblock, SIZE_BLOQUE, 1, fent);
    fread(&ext_bytemaps, SIZE_BLOQUE, 1, fent);
    fread(&ext_blq_inodos, SIZE_BLOQUE, 1, fent);
    fread(directorio, SIZE_BLOQUE, 1, fent);
    fread(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fent);

    // Bucle principal para procesar comandos
    while (1) {
        printf(">> ");
        fflush(stdin);
        fgets(comando, LONGITUD_COMANDO, stdin);

        // Procesa el comando 
        if (ComprobarComando(comando, orden, argumento1, argumento2) < 1) {
            printf("Comando no reconocido\n");
            continue;
        }

        // Comandos disponibles
        if (strcmp(orden, "info") == 0) {
            LeeSuperBloque(&ext_superblock);
        } else if (strcmp(orden, "bytemaps") == 0) {
            Printbytemaps(&ext_bytemaps);
        } else if (strcmp(orden, "dir") == 0) {
            Directorio(directorio, &ext_blq_inodos);
        } else if (strcmp(orden, "rename") == 0) {
            if (Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2) != 0) {
                printf("Error al renombrar\n");
            } else {
                printf("Archivo renombrado exitosamente\n");
            }
        } else if (strcmp(orden, "imprimir") == 0) {
            if (Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1) != 0) {
                printf("Error al imprimir\n");
            }
        } else if (strcmp(orden, "remove") == 0) {
            if (Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent) != 0) {
                printf("Error al borrar\n");
            } else {
                printf("Archivo borrado exitosamente\n");
            }
        } else if (strcmp(orden, "copy") == 0) {
            if (Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fent) != 0) {
                printf("Error al copiar\n");
            } else {
                printf("Archivo copiado exitosamente\n");
            }
        } else if (strcmp(orden, "salir") == 0) {
            GrabarSuperBloque(&ext_superblock, fent);
            GrabarByteMaps(&ext_bytemaps, fent);
            Grabarinodosydirectorio(directorio, &ext_blq_inodos, fent);
            fclose(fent);
            return 0;
        } else {
            printf("Comando desconocido\n");
        }
    }

    fclose(fent);
    return 0;
}
