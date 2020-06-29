#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return -1;
    }
    float n = atof(argv[1]);
    float scale = 1;
    if (argc > 2) {
        scale = atof(argv[2]);
    }
    float r = sqrtf(n / M_PI);
    float a = 2 * M_PI * r;
    r *= scale;
    float x = r * cosf(a);
    float y = r * sinf(a);
    printf("-x %f -y %f", x, y);
    return 0;
}
