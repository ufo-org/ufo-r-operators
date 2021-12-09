#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main (int argc,char *argv[]) {
    if (argc != 4) {
        printf("usage: gen FILE N_ITEMS s|o|r\n", argc);
        exit(1);
    }

    char *path = argv[1]; 
    size_t n = atoi(argv[2]);
    //uint32_t v = atoi(argv[3]);
    char *seq = argv[3];

    uint32_t one = 1;

    srand(42);

    FILE *fp = fopen(path, "w");
    switch (seq[0]) {
        case 'o': for (size_t i = 0; i < n; i++) {                      fwrite(&one, sizeof(uint32_t), 1, fp); } break;
        case 's': for (size_t i = 0; i < n; i++) {                      fwrite(&i,   sizeof(uint32_t), 1, fp); } break;
        case 'r': for (size_t i = 0; i < n; i++) { uint32_t r = rand(); fwrite(&r,   sizeof(uint32_t), 1, fp); } break;
    }
    fclose(fp);
}
