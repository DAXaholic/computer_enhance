#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include <cassert>
#include <string.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef double f64;

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

typedef enum
{
    Unknown,
    Error,
    ObjectStart,
    ObjectEnd,
    ArrayStart,
    ArrayEnd,
    Comma,
    Colon,
    String,
    Number
} JsonTokenType;

typedef struct
{
    const char *Data;
    u64 Length;
} JsonText;

typedef struct
{
    u64 Length;
    JsonTokenType Type;
    union
    {
        f64 Number;
        JsonText Text;
    };
} JsonToken;

typedef struct
{
    const char *Data;
    u64 Length;
    u64 Offset;
} JsonReadContext;

typedef struct
{
    f64 X0;
    f64 Y0;
    f64 X1;
    f64 Y1;
} HaversineDistance;


static u64 ReadNumber(const char *data, u64 length, f64 *number)
{
    if (length == 0) return 0;

    char firstChar = data[0];
    bool positive = firstChar != '-';
    bool firstCharIsSign = firstChar == '-' || firstChar == '+';

    f64 result = 0;
    u64 i = firstCharIsSign ? 1 : 0;
    bool foundDecimalPoint = false;
    for (; i < length; i++) {
        char c = data[i];
        if (c >= '0' && c <= '9') {
            result = result * 10 + (f64)(c - '0');
        }
        else {
            foundDecimalPoint = c == '.';
            break;
        }
    }

    if (foundDecimalPoint)
    {
        f64 factor = 0.1;
        for (i++; i < length; i++) {
            char c = data[i];
            if (c >= '0' && c <= '9') {
                result = result + factor * (f64)(c - '0');
                factor *= 0.1;
            }
            else {
                break;
            }
        }
    }

    if (!positive) result *= -1.0;
    *number = result;
    return i;
}

static bool IsWhitespaceChar(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static u64 FindNextNonWhitespaceChar(const char *data, u64 length)
{
    u64 i = 0;
    while (i < length && IsWhitespaceChar(data[i])) i++;
    return i;
}

static i64 FindEndOfJsonString(const char *data, u64 length)
{
    for (u64 i = 1; i < length; i++) {
        if (data[i] == '"' && data[i-1] != '\\') {
            return i;
        }
    }
    return -1L;
}

static bool ReadNextJsonToken(const char *data, u64 length, u64 *bytesRead, JsonToken *token)
{
    assert(bytesRead != nullptr);
    assert(token != nullptr);

    u64 nextCharIndex = FindNextNonWhitespaceChar(data, length);
    if (nextCharIndex >= length)
    {
        *bytesRead = length;
        return false;
    }
    *bytesRead = nextCharIndex + 1;

    char c = data[nextCharIndex];
    switch (c)
    {
        case '{':
        {
            token->Type = ObjectStart;
            token->Length = 1;
            break;
        }

        case '}':
        {
            token->Type = ObjectEnd;
            token->Length = 1;
            break;
        }

        case '[':
        {
            token->Type = ArrayStart;
            token->Length = 1;
            break;
        }

        case ']':
        {
            token->Type = ArrayEnd;
            token->Length = 1;
            break;
        }

        case ',':
        {
            token->Type = Comma;
            token->Length = 1;
            break;
        }

        case ':':
        {
            token->Type = Colon;
            token->Length = 1;
            break;
        }

        case '"':
        {
            u64 charsUntilEnd = FindEndOfJsonString(data + nextCharIndex, length - nextCharIndex);
            if (charsUntilEnd < 0)
            {
                token->Type = Error;
                token->Length = 0;
            }
            else
            {
                token->Type = String;
                token->Length = charsUntilEnd;
                token->Text.Data = data + nextCharIndex + 1;
                token->Text.Length = charsUntilEnd - 1;
                *bytesRead = nextCharIndex + charsUntilEnd + 1;
            }
            break;
        }

        case '-':
        case '+':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            token->Type = Number;
            token->Length = ReadNumber(data + nextCharIndex, length, &(token->Number));
            *bytesRead = nextCharIndex + token->Length;
            break;
        }

        default:
        {
            token->Type = Unknown;
            token->Length = 1;
            token->Text.Data = data + nextCharIndex;
            token->Text.Length = 1;
            break;
        }
    }

    return true;
}

static bool ReadJsonToken(JsonReadContext *context, JsonToken *token)
{
    u64 bytesRead = 0;
    bool result = ReadNextJsonToken(
        context->Data + context->Offset,
        context->Length - context->Offset,
        &bytesRead,
        token);
    context->Offset += bytesRead;
    return result;
}

i64 GetFileSize(FILE *f)
{
    _fseeki64(f, 0, SEEK_END);
    i64 ret = _ftelli64(f);
    _fseeki64(f, 0, SEEK_SET);
    return ret;
}

bool ReadHaversineJsonStart(JsonReadContext *context)
{
    JsonToken token = {};
    if (!ReadJsonToken(context, &token) || token.Type != ObjectStart) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != String) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Colon) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != ArrayStart) {
        return false;
    }
    return true;
}

bool ReadNextHaversineJsonDistance(
    JsonReadContext *context,
    HaversineDistance *haversineDistance,
    bool isFirstHaversineDistance)
{
    JsonToken token = {};

    if (!isFirstHaversineDistance &&
        (!ReadJsonToken(context, &token) || token.Type != Comma)) {
        return false;
    }

    if (!ReadJsonToken(context, &token) || token.Type != ObjectStart) {
        return false;
    }

    if (!ReadJsonToken(context, &token) ||
        token.Type != String ||
        token.Text.Length != 2 ||
        token.Text.Data[0] != 'x' ||
        token.Text.Data[1] != '0') {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Colon) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Number) {
        return false;
    }
    haversineDistance->X0 = token.Number;

    if (!ReadJsonToken(context, &token) || token.Type != Comma) {
        return false;
    }

    if (!ReadJsonToken(context, &token) ||
        token.Type != String ||
        token.Text.Length != 2 ||
        token.Text.Data[0] != 'y' ||
        token.Text.Data[1] != '0') {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Colon) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Number) {
        return false;
    }
    haversineDistance->Y0 = token.Number;

    if (!ReadJsonToken(context, &token) || token.Type != Comma) {
        return false;
    }

    if (!ReadJsonToken(context, &token) ||
        token.Type != String ||
        token.Text.Length != 2 ||
        token.Text.Data[0] != 'x' ||
        token.Text.Data[1] != '1') {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Colon) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Number) {
        return false;
    }
    haversineDistance->X1 = token.Number;

    if (!ReadJsonToken(context, &token) || token.Type != Comma) {
        return false;
    }

    if (!ReadJsonToken(context, &token) ||
        token.Type != String ||
        token.Text.Length != 2 ||
        token.Text.Data[0] != 'y' ||
        token.Text.Data[1] != '1') {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Colon) {
        return false;
    }
    if (!ReadJsonToken(context, &token) || token.Type != Number) {
        return false;
    }
    haversineDistance->Y1 = token.Number;

    if (!ReadJsonToken(context, &token) || token.Type != ObjectEnd) {
        return false;
    }

    return true;
}

void DumpJsonToken(FILE *file, const JsonToken *token)
{
    switch (token->Type)
    {
        case Unknown:
        {
            fprintf(file, "Unknown token '%c'\n", token->Text.Data[0]);
            break;
        }

        case Error:
        {
            fprintf(file, "Error\n");
            break;
        }

        case ObjectStart:
        {
            fprintf(file, "Object start\n");
            break;
        }

        case ObjectEnd:
        {
            fprintf(file, "Object end\n");
            break;
        }

        case ArrayStart:
        {
            fprintf(file, "Array start\n");
            break;
        }

        case ArrayEnd:
        {
            fprintf(file, "Array end\n");
            break;
        }

        case Comma:
        {
            fprintf(file, "Comma\n");
            break;
        }

        case Colon:
        {
            fprintf(file, "Colon\n");
            break;
        }

        case Number:
        {
            fprintf(file, "Number: ");
            fprintf(file, "%f\n", token->Number);
            break;
        }

        case String:
        {
            fprintf(file, "String: ");
            fprintf(file, "%.*s\n", (int)token->Text.Length, token->Text.Data);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s [json file name] [(opt) answer file name]\n", argv[0]);
        return 1;
    }

    FILE *jsonFile = nullptr;
    const char *jsonFileName = argv[1];
    if (fopen_s(&jsonFile, jsonFileName, "rb") != 0) {
        fprintf(stderr, "Could not open file '%s'.\n", jsonFileName);
        return 2;
    }

    f64 *answerData = nullptr;
    u64 answerDataSize = 0;
    if (argc >= 3) {
        const char *answerFileName = argv[2];
        FILE *answerFile = nullptr;
        if (fopen_s(&answerFile, answerFileName, "rb") != 0) {
            fprintf(stderr, "Could not open file '%s'.\n", answerFileName);
            return 2;
        }
        u64 answerFileSize = GetFileSize(answerFile);
        answerData = (f64 *)malloc(answerFileSize);
        answerDataSize = answerFileSize / 8;
        fread_s(answerData, answerFileSize, sizeof(f64), answerFileSize / sizeof(f64), answerFile);
        fclose(answerFile);
    }


    u64 jsonFileSize = GetFileSize(jsonFile);
    char *jsonData = (char *)malloc(jsonFileSize);
    if (jsonData == nullptr) {
        fprintf(stderr, "Could not allocate memory for JSON data.\n");
        return 3;
    }

    u64 bytesRead = fread_s(jsonData, jsonFileSize, sizeof(char), jsonFileSize, jsonFile);
    fclose(jsonFile);
    if (bytesRead != jsonFileSize)
    {
        fprintf(stderr, "Could not read whole JSON data (%lld of %lld bytes read).\n", bytesRead, jsonFileSize);
        return 4;
    }

    bool failedProcessing = false;
    JsonToken token = {};
    JsonReadContext context = {};
    context.Data = jsonData;
    context.Length = jsonFileSize;
    HaversineDistance haversineDistance;
    f64 distanceSum = 0;
    u64 distanceCount = 0;
    if (ReadHaversineJsonStart(&context))
    {
        while (ReadNextHaversineJsonDistance(&context, &haversineDistance, distanceCount == 0))
        {
            const f64 earthRadius = 6372.8;
            distanceSum += ReferenceHaversine(
                haversineDistance.X0,
                haversineDistance.Y0,
                haversineDistance.X1,
                haversineDistance.Y1,
                earthRadius);
            ++distanceCount;
        }

        if (distanceCount > 0)
        {
            fprintf(stdout, "Pairs: %llu\n", distanceCount);
            fprintf(stdout, "Avg. distance: %.16f\n", distanceSum / distanceCount);
        }
    }
    else
    {
        fprintf(stderr, "Could not parse start of haversine JSON file.\n");
    }

    if (answerData != nullptr)
    {
        fprintf(stdout, "Answer pairs: %llu\n", answerDataSize - 1);
        fprintf(stdout, "Answer avg. distance: %.16f\n", answerData[answerDataSize - 1]);
    }

    if (answerData != nullptr) free(answerData);
    if (jsonData != nullptr) free(jsonData);

    return failedProcessing ? 9 : 0;
}
