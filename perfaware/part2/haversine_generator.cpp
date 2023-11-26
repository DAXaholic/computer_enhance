#include <cstdlib>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
typedef double f64;

f64 randomF64(f64 min, f64 max)
{
    f64 range = max - min;
    f64 randPercentage = (f64)(rand()) / RAND_MAX;
    return min + range * randPercentage;
}

int writeJsonStart(FILE *dest)
{
    return fprintf(dest, "{\n  \"pairs\":[\n");
}

int writeJsonObject(FILE *dest, f64 x0, f64 y0, f64 x1, f64 y1, bool isLastOne)
{
    return fprintf(
        dest,
        "    {\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}%s\n",
        x0,
        y0,
        x1,
        y1,
        isLastOne ? "" : ",");
}

int writeJsonEnd(FILE *dest)
{
    return fprintf(dest, "  ]\n}\n");
}

static f64 Square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}

static f64 RadiansFromDegrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577 * Degrees;
    return Result;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */
    
    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    
    f64 dLat = RadiansFromDegrees(lat2 - lat1);
    f64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);
    
    f64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 Result = EarthRadius * c;
    
    return Result;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s [seed] [pairs to generate] [file name]\n", argv[0]);
        return 1;
    }
    
    const u32 seed = atoi(argv[1]);
    srand(seed);

    int numPairs = atoi(argv[2]);
    if (numPairs <= 0) {
        fprintf(stderr, "The number of pairs to generate must be a positive integer.\n");
        return 1;
    }
    
    const char *fileName = argv[3];
    FILE *dest = nullptr;
    if (fopen_s(&dest, fileName, "w") != 0) {
        fprintf(stderr, "Could not open file '%s'.", fileName);
        return 2;
    }

    const f64 earthRadius = 6372.8;
    const f64 sumFactor = 1.0 / (f64)numPairs;
    f64 avgDistance = 0;

    writeJsonStart(dest);
    for (int i = 1; i <= numPairs; i++) {
        const f64 x0 = randomF64(-180.0, 180.0);
        const f64 y0 = randomF64(-90.0, 90.0);
        const f64 x1 = randomF64(-180.0, 180.0);
        const f64 y1 = randomF64(-90.0, 90.0);
        
        writeJsonObject(dest, x0, y0, x1, y1, i == numPairs);

        const f64 distance = ReferenceHaversine(x0, y0, x1, y1, earthRadius);
        avgDistance += distance * sumFactor;
    }
    writeJsonEnd(dest);

    fprintf(stdout, "Seed: %i\n", seed);
    fprintf(stdout, "Pairs: %i\n", numPairs);
    fprintf(stdout, "Avg. distance: %f\n", avgDistance);

    fclose(dest);

    return 0;
}
