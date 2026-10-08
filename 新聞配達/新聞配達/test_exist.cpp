#include <stdio.h>
int main() {
    FILE* f;
    fopen_s(&f, "Data/house/house1.fbx", "r");
    if(f) { printf("EXISTS\n"); fclose(f); } else { printf("MISSING\n"); }
    return 0;
}
