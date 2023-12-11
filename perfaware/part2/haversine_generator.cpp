#include <cstdlib>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
typedef double f64;

f64 minF64(f64 lhs, f64 rhs) {
    return lhs < rhs ? lhs : rhs;
}

f64 maxF64(f64 lhs, f64 rhs) {
    return lhs > rhs ? lhs : rhs;
}

f64 randomF64(f64 min, f64 max)
{
    f64 range = max - min;
    f64 randPercentage = (f64)(rand()) / RAND_MAX;
    return min + range * randPercentage;
}

f64 randomF64Clustered(f64 pivot, f64 delta, f64 min, f64 max) {
    f64 effectiveMin = maxF64(min, pivot - delta);
    f64 effectiveMax = minF64(max, pivot + delta);
    return randomF64(effectiveMin, effectiveMax);
}

int writeJsonStart(FILE *dest)
{
    return fprintf(dest, "{\n  \"pairs\":[\n");
}

int writeJsonObject(FILE *dest, f64 x0, f64 y0, f64 x1, f64 y1, bool isLastOne)
{
    return fprintf(
        dest,
        "    {\"x0\":%.16f, \"y0\":%.16f, \"x1\":%.16f, \"y1\":%.16f}%s\n",
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

size_t writeF64(FILE *dest, f64 value)
{
    return fwrite((void*)&value, sizeof(f64), 1, dest);
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
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s [seed] [pairs to generate] [json file name] [(opt) answer file name]\n", argv[0]);
        return 1;
    }

    const u32 seed = atoi(argv[1]);
    srand(seed);

    int numPairs = atoi(argv[2]);
    if (numPairs <= 0) {
        fprintf(stderr, "The number of pairs to generate must be a positive integer.\n");
        return 1;
    }

    FILE *jsonFile = nullptr;
    const char *jsonFileName = argv[3];
    if (fopen_s(&jsonFile, jsonFileName, "wt") != 0) {
        fprintf(stderr, "Could not open file '%s'.", jsonFileName);
        return 2;
    }

    FILE *answerFile = nullptr;
    if (argc >= 5) {
        const char *answerFileName = argv[4];
        if (fopen_s(&answerFile, answerFileName, "wb") != 0) {
            fprintf(stderr, "Could not open file '%s'.", answerFileName);
            return 2;
        }
    }

    const f64 earthRadius = 6372.8;
    const f64 sumFactor = 1.0 / (f64)numPairs;
    f64 avgDistance = 0;
    const f64 clusterSideDelta = randomF64(15, 30);
    f64 clusterX = 0;
    f64 clusterY = 0;
    const int maxPairsPerCluster = (numPairs / 64) + 1;
    int remainingPairsInCluster = 0;

    writeJsonStart(jsonFile);
    for (int i = 1; i <= numPairs; i++) {
        if (remainingPairsInCluster == 0) {
            clusterX = randomF64(-180.0, 180);
            clusterY = randomF64(-90, 90);
            remainingPairsInCluster = maxPairsPerCluster;
        }
        remainingPairsInCluster--;

        const f64 x0 = randomF64Clustered(clusterX, clusterSideDelta, -180.0, 180.0);
        const f64 y0 = randomF64Clustered(clusterY, clusterSideDelta, -90.0, 90.0);
        const f64 x1 = randomF64Clustered(clusterX, clusterSideDelta, -180.0, 180.0);
        const f64 y1 = randomF64Clustered(clusterY, clusterSideDelta, -90.0, 90.0);
        writeJsonObject(jsonFile, x0, y0, x1, y1, i == numPairs);

        const f64 distance = ReferenceHaversine(x0, y0, x1, y1, earthRadius);
        if (answerFile != nullptr) {
            writeF64(answerFile, distance);
        }

        avgDistance += distance * sumFactor;
    }
    writeF64(answerFile, avgDistance);
    writeJsonEnd(jsonFile);

    fclose(answerFile);
    fclose(jsonFile);

    fprintf(stdout, "Seed: %i\n", seed);
    fprintf(stdout, "Pairs: %i\n", numPairs);
    fprintf(stdout, "Avg. distance: %.16f\n", avgDistance);

    return 0;
}
