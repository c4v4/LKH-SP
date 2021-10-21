#include "LKH.h"

#define CALLOC(ptr, nelem, TYPE) assert((ptr = (TYPE *)calloc(nelem, sizeof(TYPE))))
#define MALLOC(ptr, nelem, TYPE) assert((ptr = (TYPE *)malloc((nelem) * sizeof(TYPE))))
#define REALLOC(ptr, nelem, TYPE) assert((ptr = (TYPE *)realloc(ptr, (nelem) * sizeof(TYPE))))

#define FREEN0(a) \
    do {          \
        free(a);  \
        a = NULL; \
    } while (0)

#define MAX(a, b) ((a) >= (b) ? (a) : (b))


/**
 * Given a file path, return the file name withot extension
 * and without the path.
 */
char *strip_path_and_extension(const char *filePath) {
    if (!filePath)
        return NULL;

    const char *subs_start = MAX(strrchr(filePath, '/'), strrchr(filePath, '\\'));
    subs_start = MAX(subs_start, filePath) + 1;

    const char *subs_end = strrchr(filePath, '.');
    if (!subs_end)
        subs_end = filePath + strlen(filePath);

    size_t subs_length = subs_end - subs_start;
    char *fileName = NULL;
    CALLOC(fileName, subs_length + 1, char);
    strncpy(fileName, subs_start, subs_length);
    return fileName;
}

/**
 * Starting from an instance file path, return a output Tour file name ending
 * with ".$.sol", where the char $ will be replaced by the Tour cost if
 * writed using LKH function.
 */
int tour_file_name_with_length(char **dest, const char *src) {
    if (!src)
        return 1;

    char *basename = strip_path_and_extension(src);
    CALLOC(*dest, strlen(basename) + 7, char);
    strcat(strcpy(*dest, basename), ".$.sol");

    FREEN0(basename);
    return 0;
}

void WriteSolFile(int *Tour, GainType Cost) {
    FILE *ResultFile;
    char *FullFileName;
    int i = 0;
    time_t Now;

    char *tempFileName = NULL;
    tour_file_name_with_length(&tempFileName, ProblemFileName);
    FullFileName = FullName(tempFileName, Cost);
    Now = time(&Now);

    if (TraceLevel >= 1)
        printff("Writing CVRPLIB solution file: \"%s\" ... ", FullFileName);

    ResultFile = fopen(FullFileName, "w");
    assert(ResultFile);
    while (NodeSet[Tour[i]].DepotId == 0)
        ++i;
    ++i;
    assert(NodeSet[Tour[i]].DepotId == 0);
    int end = i + DimensionSaved;

    for (int Route = 1; Route <= Salesmen; ++Route) {
        fprintf(ResultFile, "Route #%d: ", Route);
        assert(NodeSet[Tour[i]].DepotId == 0);
        for (int mod_i = i % DimensionSaved; i < end && NodeSet[Tour[mod_i]].DepotId == 0; mod_i = ++i % DimensionSaved) {
            fprintf(ResultFile, "%d ", Tour[mod_i] - 1);
        }
        fprintf(ResultFile, "\n");
        ++i;
    }

    fprintf(ResultFile, "Cost " GainFormat "\n", Cost);
    fclose(ResultFile);
    if (TraceLevel >= 1)
        printff("done.\n");
}