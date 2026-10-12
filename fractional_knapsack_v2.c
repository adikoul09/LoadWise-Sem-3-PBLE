/*
 * ============================================================================
 *  Analysis of Algorithms (AOA) - PBLE 1                         [Version 2]
 *  Smart Delivery Planning - Fractional Knapsack (Greedy Method)
 * ============================================================================
 *  A delivery company has vehicle(s) with fixed carrying capacity. Each
 *  package has a value (profit) and a weight. A package may be taken
 *  completely or fractionally. Goal: maximise the total value carried.
 *
 *  GREEDY STRATEGY
 *    1. Compute ratio = value / weight for every package.
 *    2. Sort packages in DECREASING order of ratio (stable sort).
 *    3. Walk down the sorted list:
 *         - if the whole package fits   -> take it completely (fraction 1.0)
 *         - otherwise                   -> take only the fraction that fills
 *                                          the remaining capacity.
 *
 *  FEATURES ADDED IN THIS VERSION (the 7-option menu is unchanged)
 *    AOA content
 *      [1] Merge sort option + comparison/move counters; "compare both" mode
 *      [2] 0/1 Knapsack comparison using dynamic programming (option 5)
 *    Delivery-tool features
 *      [3] Package names shown everywhere
 *      [4] Multiple vehicles (1 to 5) loaded one after another
 *    Usability
 *      [5] Save / load package data to a text file (option 1 sub-menu)
 *      [6] Export the selected-packages report to a text file (option 6)
 *    Visual extras
 *      [7] ASCII load bar for every vehicle, showing which package filled it
 *      [8] Optional step-by-step mode: pause after each greedy step (option 5)
 *
 *  TIME COMPLEXITY (n packages, k vehicles, W = capacity for 0/1 DP)
 *    Ratio calculation ............ O(n)
 *    Bubble sort .................. O(n^2)       Merge sort ... O(n log n)
 *    Greedy selection scan ........ O(n + k)
 *    Overall ...................... O(n^2) with bubble sort,
 *                                   O(n log n) with merge sort
 *    0/1 comparison (optional) .... O(n * W)
 *    Space complexity: O(n) (O(n * W) for the optional 0/1 table).
 *
 *  Compile :  gcc -Wall -Wextra -std=c99 -o knapsack fractional_knapsack_v2.c
 *  Run     :  ./knapsack
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PACKAGES        50       /* maximum number of packages          */
#define MAX_VEHICLES        5        /* maximum number of vehicles          */
#define NAME_LEN            24       /* package name buffer (incl. '\0')    */
#define LINE_SIZE           256      /* buffer size for one line of input   */
#define EPSILON             1e-9     /* tolerance for double comparisons    */
#define MAX_INPUT_NUMBER    1e9      /* upper limit accepted for any number */
#define MAX_DP_CAPACITY     10000    /* largest capacity for the 0/1 DP     */
#define BAR_WIDTH           40       /* characters in the ASCII load bar    */
#define DEFAULT_DATA_FILE   "knapsack_data.txt"
#define DEFAULT_REPORT_FILE "knapsack_report.txt"

/* ---------------------------------------------------------------------------
 * Data structures
 * ------------------------------------------------------------------------- */
typedef struct {
    int    number;                        /* package number 1..n (fixed)    */
    char   name[NAME_LEN];                /* descriptive name               */
    double value;                         /* value / profit                 */
    double weight;                        /* weight                         */
    double ratio;                         /* value / weight                 */
    double fraction;                      /* total fraction taken (0..1)    */
    double vehicleFraction[MAX_VEHICLES]; /* fraction put in each vehicle   */
} Package;

typedef struct {
    long comparisons;                     /* ratio comparisons performed    */
    long moves;                           /* swaps (bubble) / writes (merge)*/
} SortStats;

/* ---------------------------------------------------------------------------
 * Program state (fixed-size global arrays -> no dynamic memory, no leaks)
 * ------------------------------------------------------------------------- */
static Package packages[MAX_PACKAGES];
static int     packageCount = 0;                       /* n                 */
static int     vehicleCount = 1;                       /* k                 */
static double  vehicleCapacity[MAX_VEHICLES];          /* capacity of each  */

static int     dataEntered      = 0;   /* 1 once package data exists        */
static int     ratiosCalculated = 0;   /* 1 once ratios are computed        */
static int     sortedByRatio    = 0;   /* 1 once list is sorted             */
static int     knapsackSolved   = 0;   /* 1 once greedy selection is done   */

static double  maxValue        = 0.0;                  /* result: max value */
static double  totalWeightUsed = 0.0;                  /* result: weight    */
static double  vehicleValue[MAX_VEHICLES];             /* value per vehicle */
static double  vehicleWeight[MAX_VEHICLES];            /* weight per vehicle*/

static int       sortMethodUsed = 0;   /* 0 = none, 1 = bubble, 2 = merge   */
static SortStats lastSortStats;        /* counters of the last sort         */
static int       comparedBoth   = 0;   /* 1 if "compare both" was run       */
static SortStats bubbleStats;          /* counters from "compare both"      */
static SortStats mergeStats;

static Package   mergeBuffer[MAX_PACKAGES];   /* scratch space for merge    */
static Package   workList[MAX_PACKAGES];      /* scratch copy of the list   */
static Package   workList2[MAX_PACKAGES];     /* second scratch copy        */
static double    dpTable[MAX_PACKAGES + 1][MAX_DP_CAPACITY + 1]; /* 0/1 DP  */

/* ===========================================================================
 *  SAFE INPUT HELPERS  (fgets + strtol/strtod => no scanf overflow problems)
 * ========================================================================= */

/* Reads one line (without the newline). Over-long lines are discarded to the
 * end so the leftover never leaks into the next prompt. 0 = EOF. */
static int readLine(char *buffer, size_t size)
{
    size_t length;
    int    ch;

    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* discard the rest of an over-long line */
        }
    }
    if (length > 0 && buffer[length - 1] == '\r') {
        buffer[length - 1] = '\0';
    }
    return 1;
}

/* If stdin is exhausted, end the program politely instead of looping forever */
static void exitOnEof(void)
{
    if (feof(stdin)) {
        printf("\nEnd of input reached. Exiting.\n");
        exit(EXIT_SUCCESS);
    }
}

/* Strict number parsers: the whole text must be a number (spaces allowed) */
static int parseDouble(const char *text, double *result)
{
    char  *end;
    double number = strtod(text, &end);

    if (end == text) {
        return 0;
    }
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
        end++;
    }
    if (*end != '\0') {
        return 0;
    }
    *result = number;
    return 1;
}

static int parseLong(const char *text, long *result)
{
    char *end;
    long  number = strtol(text, &end, 10);

    if (end == text) {
        return 0;
    }
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
        end++;
    }
    if (*end != '\0') {
        return 0;
    }
    *result = number;
    return 1;
}

static int isValidPositive(double number)
{
    return number > 0.0 && number <= MAX_INPUT_NUMBER;   /* also rejects NaN */
}

/* Reads an integer from stdin. Returns 1 on success, 0 on bad input / EOF */
static int readInt(int *result)
{
    char line[LINE_SIZE];
    long number;

    if (!readLine(line, sizeof line) || !parseLong(line, &number)) {
        return 0;
    }
    if (number < -1000000L || number > 1000000L) {
        return 0;
    }
    *result = (int)number;
    return 1;
}

/* Keeps asking until the user enters a number greater than 0 */
static double readPositiveDouble(const char *prompt)
{
    char   line[LINE_SIZE];
    double number;

    for (;;) {
        printf("%s", prompt);
        if (readLine(line, sizeof line) && parseDouble(line, &number)
            && isValidPositive(number)) {
            return number;
        }
        exitOnEof();
        printf("  [!] Invalid input. Enter a number greater than 0.\n");
    }
}

/* Asks a yes/no question. Anything except y/Y (or EOF) counts as "no" */
static int askYesNo(const char *prompt)
{
    char line[LINE_SIZE];

    printf("%s", prompt);
    if (!readLine(line, sizeof line)) {
        return 0;
    }
    return line[0] == 'y' || line[0] == 'Y';
}

/* Copies a name, replacing commas (they separate fields in the data file) */
static void copyName(char *destination, const char *source)
{
    int i;

    for (i = 0; i < NAME_LEN - 1 && source[i] != '\0'; i++) {
        destination[i] = (source[i] == ',') ? ' ' : source[i];
    }
    destination[i] = '\0';
}

static double absDouble(double x)
{
    return x < 0.0 ? -x : x;
}

/* ===========================================================================
 *  STATE HELPERS
 * ========================================================================= */

static double totalCapacity(void)
{
    double total = 0.0;
    int    v;

    for (v = 0; v < vehicleCount; v++) {
        total += vehicleCapacity[v];
    }
    return total;
}

/* Forget the greedy selection (fractions and totals) */
static void clearSelections(void)
{
    int i;
    int v;

    for (i = 0; i < packageCount; i++) {
        packages[i].fraction = 0.0;
        for (v = 0; v < MAX_VEHICLES; v++) {
            packages[i].vehicleFraction[v] = 0.0;
        }
    }
    for (v = 0; v < MAX_VEHICLES; v++) {
        vehicleValue[v]  = 0.0;
        vehicleWeight[v] = 0.0;
    }
    maxValue        = 0.0;
    totalWeightUsed = 0.0;
    knapsackSolved  = 0;
}

/* Forget every result that depends on the package data */
static void resetResults(void)
{
    int i;

    for (i = 0; i < packageCount; i++) {
        packages[i].ratio = 0.0;
    }
    clearSelections();
    ratiosCalculated = 0;
    sortedByRatio    = 0;
    sortMethodUsed   = 0;
    comparedBoth     = 0;
}

static int requireData(void)
{
    if (!dataEntered) {
        printf("\n[!] No package data yet. Use option 1 first.\n");
        return 0;
    }
    return 1;
}

/* Prints the vehicle capacity line(s) to the given stream */
static void printVehicleInfo(FILE *out)
{
    int v;

    if (vehicleCount == 1) {
        fprintf(out, "Vehicle capacity = %.2f\n", vehicleCapacity[0]);
    } else {
        fprintf(out, "Vehicles = %d | capacities:", vehicleCount);
        for (v = 0; v < vehicleCount; v++) {
            fprintf(out, " V%d=%.2f", v + 1, vehicleCapacity[v]);
        }
        fprintf(out, " | total = %.2f\n", totalCapacity());
    }
}

/* Puts a copy of the package list into original input order (by number) */
static void copyInInputOrder(Package destination[])
{
    int     i;
    int     j;
    Package key;

    for (i = 0; i < packageCount; i++) {
        destination[i] = packages[i];
    }
    for (i = 1; i < packageCount; i++) {              /* insertion sort     */
        key = destination[i];
        j   = i - 1;
        while (j >= 0 && destination[j].number > key.number) {
            destination[j + 1] = destination[j];
            j--;
        }
        destination[j + 1] = key;
    }
}

/* ===========================================================================
 *  OPTION 1 : ENTER PACKAGE DETAILS  (manual / samples / file load / save)
 * ========================================================================= */

/* Hard-coded illustrative example:
 *   Package | Name        | Value | Weight        Capacity = 15
 *      1    | Documents   |  40   |   5
 *      2    | Furniture   |  30   |  10
 *      3    | Medical Kit |  50   |   5
 *      4    | Electronics |  20   |   4
 * With vehicles == 2 the same packages are loaded into two vehicles of
 * capacity 8 and 7 (same total capacity, so the same maximum value). */
static void loadSampleTestCase(int vehicles)
{
    static const char  *sampleName[]   = { "Documents", "Furniture",
                                           "Medical Kit", "Electronics" };
    static const double sampleValue[]  = { 40.0, 30.0, 50.0, 20.0 };
    static const double sampleWeight[] = {  5.0, 10.0,  5.0,  4.0 };
    int i;

    packageCount = 4;
    for (i = 0; i < packageCount; i++) {
        packages[i].number = i + 1;
        copyName(packages[i].name, sampleName[i]);
        packages[i].value  = sampleValue[i];
        packages[i].weight = sampleWeight[i];
    }
    if (vehicles == 2) {
        vehicleCount       = 2;
        vehicleCapacity[0] = 8.0;
        vehicleCapacity[1] = 7.0;
    } else {
        vehicleCount       = 1;
        vehicleCapacity[0] = 15.0;
    }
    dataEntered = 1;
    resetResults();
    printf("\nSample test case loaded: 4 packages, %d vehicle(s), total capacity = 15.\n",
           vehicleCount);
}

static void enterManually(void)
{
    int  n;
    int  k;
    int  i;
    int  v;
    char prompt[LINE_SIZE];
    char nameBuffer[LINE_SIZE];

    for (;;) {                                         /* number of packages */
        printf("Enter number of packages (1 to %d): ", MAX_PACKAGES);
        if (readInt(&n) && n >= 1 && n <= MAX_PACKAGES) {
            break;
        }
        exitOnEof();
        printf("  [!] Invalid input. Enter an integer between 1 and %d.\n",
               MAX_PACKAGES);
    }
    packageCount = n;

    for (i = 0; i < packageCount; i++) {               /* name/value/weight  */
        packages[i].number = i + 1;

        printf("Package %d - name (Enter for default): ", i + 1);
        readLine(nameBuffer, sizeof nameBuffer);
        exitOnEof();
        if (nameBuffer[0] == '\0') {
            snprintf(nameBuffer, sizeof nameBuffer, "Package %d", i + 1);
        }
        copyName(packages[i].name, nameBuffer);

        snprintf(prompt, sizeof prompt, "Package %d - value/profit  : ", i + 1);
        packages[i].value = readPositiveDouble(prompt);

        snprintf(prompt, sizeof prompt, "Package %d - weight        : ", i + 1);
        packages[i].weight = readPositiveDouble(prompt);
    }

    for (;;) {                                         /* number of vehicles */
        printf("Enter number of vehicles (1 to %d): ", MAX_VEHICLES);
        if (readInt(&k) && k >= 1 && k <= MAX_VEHICLES) {
            break;
        }
        exitOnEof();
        printf("  [!] Invalid input. Enter an integer between 1 and %d.\n",
               MAX_VEHICLES);
    }
    vehicleCount = k;

    for (v = 0; v < vehicleCount; v++) {               /* capacities         */
        if (vehicleCount == 1) {
            snprintf(prompt, sizeof prompt,
                     "Enter maximum carrying capacity of the vehicle: ");
        } else {
            snprintf(prompt, sizeof prompt,
                     "Enter carrying capacity of vehicle %d: ", v + 1);
        }
        vehicleCapacity[v] = readPositiveDouble(prompt);
    }

    dataEntered = 1;
    resetResults();
    printf("\n%d package(s) and %d vehicle(s) stored.\n", packageCount, vehicleCount);
    printVehicleInfo(stdout);
}

/* Returns the next non-empty, non-comment line of a data file. 0 = no more */
static int nextDataLine(FILE *fp, char *line, size_t size)
{
    size_t length;

    while (fgets(line, (int)size, fp) != NULL) {
        length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[--length] = '\0';
        }
        if (length == 0 || line[0] == '#') {
            continue;
        }
        return 1;
    }
    return 0;
}

/* Saves the current data (in original input order) to a text file */
static void saveDataToFile(void)
{
    char  filename[LINE_SIZE];
    FILE *fp;
    int   i;
    int   v;

    if (!requireData()) {
        return;
    }
    printf("File name to save to [%s]: ", DEFAULT_DATA_FILE);
    readLine(filename, sizeof filename);
    if (filename[0] == '\0') {
        snprintf(filename, sizeof filename, "%s", DEFAULT_DATA_FILE);
    }

    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("[!] Could not open '%s' for writing.\n", filename);
        return;
    }

    copyInInputOrder(workList);
    fprintf(fp, "# Fractional Knapsack data file (records are: name,value,weight)\n");
    fprintf(fp, "FKDATA\n%d\n%d\n", packageCount, vehicleCount);
    for (v = 0; v < vehicleCount; v++) {
        fprintf(fp, "%s%.10g", v == 0 ? "" : " ", vehicleCapacity[v]);
    }
    fprintf(fp, "\n");
    for (i = 0; i < packageCount; i++) {
        fprintf(fp, "%s,%.10g,%.10g\n", workList[i].name,
                workList[i].value, workList[i].weight);
    }
    if (fclose(fp) != 0) {
        printf("[!] Error while writing '%s'.\n", filename);
        return;
    }
    printf("Data saved to '%s'.\n", filename);
}

/* Loads data from a text file written by saveDataToFile().
 * Everything is validated into temporary variables first, so a bad file never
 * damages the data that is already in memory. */
static void loadDataFromFile(void)
{
    char    filename[LINE_SIZE];
    char    line[LINE_SIZE];
    FILE   *fp;
    long    n;
    long    k;
    int     i;
    int     v;
    char   *cursor;
    char   *end;
    char   *comma;
    double  capacities[MAX_VEHICLES];
    double  number;

    printf("File name to load from [%s]: ", DEFAULT_DATA_FILE);
    readLine(filename, sizeof filename);
    if (filename[0] == '\0') {
        snprintf(filename, sizeof filename, "%s", DEFAULT_DATA_FILE);
    }
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("[!] Could not open '%s'.\n", filename);
        return;
    }

    /* header, n, k */
    if (!nextDataLine(fp, line, sizeof line) || strcmp(line, "FKDATA") != 0) {
        printf("[!] '%s' is not a valid data file (missing FKDATA header).\n", filename);
        fclose(fp);
        return;
    }
    if (!nextDataLine(fp, line, sizeof line) || !parseLong(line, &n)
        || n < 1 || n > MAX_PACKAGES) {
        printf("[!] Invalid package count in file (allowed: 1 to %d).\n", MAX_PACKAGES);
        fclose(fp);
        return;
    }
    if (!nextDataLine(fp, line, sizeof line) || !parseLong(line, &k)
        || k < 1 || k > MAX_VEHICLES) {
        printf("[!] Invalid vehicle count in file (allowed: 1 to %d).\n", MAX_VEHICLES);
        fclose(fp);
        return;
    }

    /* capacities: k numbers on one line */
    if (!nextDataLine(fp, line, sizeof line)) {
        printf("[!] Capacity line missing in file.\n");
        fclose(fp);
        return;
    }
    cursor = line;
    for (v = 0; v < (int)k; v++) {
        number = strtod(cursor, &end);
        if (end == cursor || !isValidPositive(number)) {
            printf("[!] Invalid vehicle capacity in file.\n");
            fclose(fp);
            return;
        }
        capacities[v] = number;
        cursor = end;
    }
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
    if (*cursor != '\0') {
        printf("[!] Unexpected extra text on the capacity line.\n");
        fclose(fp);
        return;
    }

    /* package records: name,value,weight */
    for (i = 0; i < (int)n; i++) {
        double value;
        double weight;

        if (!nextDataLine(fp, line, sizeof line)) {
            printf("[!] File ended early: expected %ld package records.\n", n);
            fclose(fp);
            return;
        }
        comma = strrchr(line, ',');
        if (comma == NULL || !parseDouble(comma + 1, &weight) || !isValidPositive(weight)) {
            printf("[!] Invalid weight in record %d.\n", i + 1);
            fclose(fp);
            return;
        }
        *comma = '\0';
        comma = strrchr(line, ',');
        if (comma == NULL || !parseDouble(comma + 1, &value) || !isValidPositive(value)) {
            printf("[!] Invalid value in record %d.\n", i + 1);
            fclose(fp);
            return;
        }
        *comma = '\0';
        workList[i].number = i + 1;
        if (line[0] == '\0') {
            snprintf(line, sizeof line, "Package %d", i + 1);
        }
        copyName(workList[i].name, line);
        workList[i].value  = value;
        workList[i].weight = weight;
    }
    fclose(fp);

    /* everything is valid: commit to the real data */
    packageCount = (int)n;
    vehicleCount = (int)k;
    for (v = 0; v < vehicleCount; v++) {
        vehicleCapacity[v] = capacities[v];
    }
    for (i = 0; i < packageCount; i++) {
        packages[i] = workList[i];
    }
    dataEntered = 1;
    resetResults();
    printf("\nLoaded %d package(s) and %d vehicle(s) from '%s'.\n",
           packageCount, vehicleCount, filename);
    printVehicleInfo(stdout);
}

static void enterPackages(void)
{
    int choice;

    printf("\n--- ENTER PACKAGE DETAILS ---\n");
    printf("  1. Enter data manually\n");
    printf("  2. Load the illustrative example (4 packages, 1 vehicle, capacity 15)\n");
    printf("  3. Load the example with 2 vehicles (capacities 8 and 7)\n");
    printf("  4. Load data from a file\n");
    printf("  5. Save current data to a file\n");
    printf("Choice: ");
    if (!readInt(&choice) || choice < 1 || choice > 5) {
        printf("[!] Invalid choice. Returning to main menu.\n");
        return;
    }
    switch (choice) {
    case 1: enterManually();        break;
    case 2: loadSampleTestCase(1);  break;
    case 3: loadSampleTestCase(2);  break;
    case 4: loadDataFromFile();     break;
    default: saveDataToFile();      break;
    }
}

/* ===========================================================================
 *  OPTION 2 / 3 : DISPLAY DETAILS, CALCULATE RATIO
 * ========================================================================= */

static void displayPackages(const char *title)
{
    int i;

    printf("\n%s\n", title);
    printVehicleInfo(stdout);
    printf("+---------+------------------+----------+----------+-----------------+\n");
    printf("| Package | Name             |  Value   |  Weight  | Value/Weight    |\n");
    printf("+---------+------------------+----------+----------+-----------------+\n");
    for (i = 0; i < packageCount; i++) {
        if (ratiosCalculated) {
            printf("| %7d | %-16.16s | %8.2f | %8.2f | %15.4f |\n",
                   packages[i].number, packages[i].name, packages[i].value,
                   packages[i].weight, packages[i].ratio);
        } else {
            printf("| %7d | %-16.16s | %8.2f | %8.2f | %15s |\n",
                   packages[i].number, packages[i].name, packages[i].value,
                   packages[i].weight, "(not computed)");
        }
    }
    printf("+---------+------------------+----------+----------+-----------------+\n");
}

/* Value/Weight ratio for every package   -> O(n) */
static void calculateRatio(void)
{
    int i;

    for (i = 0; i < packageCount; i++) {
        packages[i].ratio = packages[i].value / packages[i].weight;
    }
    ratiosCalculated = 1;
    sortedByRatio    = 0;     /* the old order is no longer known to be sorted */
    clearSelections();

    printf("\nStep: Value/Weight ratio = Value / Weight, computed for each package.\n");
    for (i = 0; i < packageCount; i++) {
        printf("  Package %d (%s): %.2f / %.2f = %.4f\n",
               packages[i].number, packages[i].name, packages[i].value,
               packages[i].weight, packages[i].ratio);
    }
}

/* ===========================================================================
 *  OPTION 4 : SORTING   (bubble sort and merge sort, both STABLE)
 * ========================================================================= */

/* 1 when package a must come AFTER package b (smaller ratio). Equal ratios
 * return 0 so equal packages never change their relative order => stable. */
static int ratioLess(const Package *a, const Package *b)
{
    return a->ratio < b->ratio - EPSILON;
}

/* Bubble sort, decreasing ratio.   Time: O(n^2) */
static void bubbleSort(Package list[], int n, SortStats *stats)
{
    int     pass;
    int     j;
    int     swapped;
    Package temp;

    for (pass = 0; pass < n - 1; pass++) {
        swapped = 0;
        for (j = 0; j < n - 1 - pass; j++) {
            stats->comparisons++;
            if (ratioLess(&list[j], &list[j + 1])) {
                temp         = list[j];
                list[j]      = list[j + 1];
                list[j + 1]  = temp;
                stats->moves++;
                swapped = 1;
            }
        }
        if (!swapped) {
            break;                    /* already sorted: early exit          */
        }
    }
}

/* Merge sort on list[low .. high-1], decreasing ratio.   Time: O(n log n) */
static void mergeSortRange(Package list[], int low, int high, SortStats *stats)
{
    int mid;
    int i;
    int j;
    int k;

    if (high - low < 2) {
        return;
    }
    mid = low + (high - low) / 2;
    mergeSortRange(list, low, mid, stats);
    mergeSortRange(list, mid, high, stats);

    i = low;
    j = mid;
    k = low;
    while (i < mid && j < high) {
        stats->comparisons++;
        if (ratioLess(&list[i], &list[j])) {
            mergeBuffer[k++] = list[j++];
        } else {
            mergeBuffer[k++] = list[i++];      /* ties take the left element */
        }
        stats->moves++;
    }
    while (i < mid) {
        mergeBuffer[k++] = list[i++];
        stats->moves++;
    }
    while (j < high) {
        mergeBuffer[k++] = list[j++];
        stats->moves++;
    }
    for (k = low; k < high; k++) {
        list[k] = mergeBuffer[k];
    }
}

static void printSortStats(const char *methodName, const SortStats *stats)
{
    printf("  %-12s : %ld comparisons, %ld %s\n", methodName, stats->comparisons,
           stats->moves, strcmp(methodName, "Bubble sort") == 0 ? "swaps" : "element writes");
}

/* Sorts the real package list with the chosen method (1 = bubble, 2 = merge) */
static void sortPackages(int method)
{
    SortStats stats;

    stats.comparisons = 0;
    stats.moves       = 0;
    if (method == 2) {
        mergeSortRange(packages, 0, packageCount, &stats);
    } else {
        bubbleSort(packages, packageCount, &stats);
    }
    sortedByRatio  = 1;
    sortMethodUsed = (method == 2) ? 2 : 1;
    lastSortStats  = stats;
    comparedBoth   = 0;
    clearSelections();
}

/* Runs both algorithms on the ORIGINAL input order and prints their counters;
 * the real list ends up sorted (by the merge sort result). */
static void compareSortMethods(void)
{
    int i;

    copyInInputOrder(workList);
    for (i = 0; i < packageCount; i++) {
        workList2[i] = workList[i];
    }
    bubbleStats.comparisons = bubbleStats.moves = 0;
    mergeStats.comparisons  = mergeStats.moves  = 0;

    bubbleSort(workList, packageCount, &bubbleStats);
    mergeSortRange(workList2, 0, packageCount, &mergeStats);

    for (i = 0; i < packageCount; i++) {
        packages[i] = workList2[i];
    }
    sortedByRatio  = 1;
    sortMethodUsed = 2;
    lastSortStats  = mergeStats;
    comparedBoth   = 1;
    clearSelections();

    printf("\nSORTING COMPARISON on %d packages (input order, same data):\n", packageCount);
    printSortStats("Bubble sort", &bubbleStats);
    printSortStats("Merge sort", &mergeStats);
    printf("  Bubble sort is O(n^2); merge sort is O(n log n). The gap grows quickly\n");
    printf("  with n (for tiny lists the difference is small).\n");
}

static void sortMenu(void)
{
    int choice;

    if (!ratiosCalculated) {
        printf("\n(Ratios were not calculated yet - calculating them now.)");
        calculateRatio();
    }
    printf("\n--- SORT PACKAGES BY RATIO (decreasing) ---\n");
    printf("  1. Bubble sort  - O(n^2)\n");
    printf("  2. Merge sort   - O(n log n)\n");
    printf("  3. Compare both (shows operation counts)\n");
    printf("Choice: ");
    if (!readInt(&choice) || choice < 1 || choice > 3) {
        printf("[!] Invalid choice. Returning to main menu.\n");
        return;
    }

    if (choice == 3) {
        compareSortMethods();
        printf("\nStep: Packages sorted in DECREASING order of Value/Weight ratio"
               " (stable merge sort result kept).\n");
    } else {
        sortPackages(choice);
        printf("\nStep: Packages sorted in DECREASING order of Value/Weight ratio"
               " (stable %s).\n", choice == 2 ? "merge sort" : "bubble sort");
        printSortStats(choice == 2 ? "Merge sort" : "Bubble sort", &lastSortStats);
    }
    displayPackages("Sorted package list:");
}

/* ===========================================================================
 *  GREEDY FRACTIONAL KNAPSACK  (works for 1 or several vehicles)
 * ========================================================================= */

/* Lets the user read each greedy step before the next one appears */
static void pauseForStep(int stepMode)
{
    char line[LINE_SIZE];

    if (stepMode) {
        printf("      [Press Enter for the next step]");
        readLine(line, sizeof line);
        printf("\n");
    }
}

/* Vehicles are loaded one after another. A package that does not fit in the
 * space left in the current vehicle is split: the part that fits goes into
 * this vehicle and the rest continues into the next one. For one vehicle this
 * is exactly the classic fractional knapsack.   Time: O(n + k) after sorting */
static void fractionalKnapsack(int stepMode)
{
    double remaining[MAX_VEHICLES];
    int    v = 0;
    int    i;

    clearSelections();
    for (i = 0; i < vehicleCount; i++) {
        remaining[i] = vehicleCapacity[i];
    }

    printf("\nGREEDY SELECTION\n");
    printVehicleInfo(stdout);
    printf("--------------------------------------------------------------\n");

    for (i = 0; i < packageCount; i++) {
        double leftFraction = 1.0;     /* part of this package still to place */
        int    placedAny    = 0;

        while (leftFraction > EPSILON) {
            double need;
            double amountFraction;     /* fraction placed in this step        */
            double weightPlaced;
            double valueGained;

            /* move to the next vehicle that still has room */
            while (v < vehicleCount && remaining[v] <= EPSILON) {
                if (v + 1 < vehicleCount) {
                    printf("  -> Vehicle %d is full. Switching to vehicle %d.\n",
                           v + 1, v + 2);
                }
                v++;
            }
            if (v >= vehicleCount) {
                break;                                  /* every vehicle full */
            }

            need = leftFraction * packages[i].weight;   /* weight still to carry */

            if (need <= remaining[v] + EPSILON) {
                /* the whole (remaining part of the) package fits */
                amountFraction = leftFraction;
                weightPlaced   = need;
            } else {
                /* it does not fit: take only what fits in this vehicle */
                weightPlaced   = remaining[v];
                amountFraction = weightPlaced / packages[i].weight;
            }
            valueGained = amountFraction * packages[i].value;

            packages[i].vehicleFraction[v] += amountFraction;
            packages[i].fraction           += amountFraction;
            vehicleValue[v]   += valueGained;
            vehicleWeight[v]  += weightPlaced;
            totalWeightUsed   += weightPlaced;
            maxValue          += valueGained;
            remaining[v]      -= weightPlaced;
            if (remaining[v] < EPSILON) {
                remaining[v] = 0.0;
            }

            printf("Package %d (%s, ratio %.4f)", packages[i].number,
                   packages[i].name, packages[i].ratio);
            if (vehicleCount > 1) {
                printf(" -> vehicle %d", v + 1);
            }
            printf(": ");
            if (need <= weightPlaced + EPSILON && leftFraction >= 1.0 - EPSILON) {
                printf("weight %.2f <= remaining -> take FULL.", packages[i].weight);
            } else if (need <= weightPlaced + EPSILON) {
                printf("rest (weight %.2f) fits -> take the REMAINING %.4f.",
                       weightPlaced, amountFraction);
            } else {
                printf("%s %.2f > remaining %.2f -> take FRACTION %.4f (%.2f/%.2f).",
                       leftFraction >= 1.0 - EPSILON ? "weight" : "rest",
                       need, weightPlaced, amountFraction, weightPlaced,
                       packages[i].weight);
            }
            printf(" Value +%.2f, remaining capacity = %.2f\n", valueGained, remaining[v]);
            pauseForStep(stepMode);

            leftFraction -= amountFraction;
            placedAny = 1;
        }

        if (!placedAny) {
            printf("Package %d (%s): all vehicles are full -> skipped.\n",
                   packages[i].number, packages[i].name);
            pauseForStep(stepMode);
        }
    }
    printf("--------------------------------------------------------------\n");
    knapsackSolved = 1;
}

/* ===========================================================================
 *  0/1 KNAPSACK COMPARISON  (dynamic programming, no splitting allowed)
 * ========================================================================= */

/* Shows what the best answer would be if packages could NOT be split.
 * Needs whole-number weights/capacity and a single vehicle.   Time: O(n * W) */
static void compareWith01Knapsack(void)
{
    int    chosen[MAX_PACKAGES];
    int    capacity;
    int    i;
    int    c;
    int    anyChosen = 0;
    double chosenWeight = 0.0;
    double best;

    printf("\nCOMPARISON WITH 0/1 KNAPSACK (packages cannot be split)\n");

    if (vehicleCount != 1) {
        printf("  Skipped: this comparison is available for a single vehicle only.\n");
        return;
    }
    if (vehicleCapacity[0] > MAX_DP_CAPACITY + 0.5
        || absDouble(vehicleCapacity[0] - (double)(long)(vehicleCapacity[0] + 0.5)) > EPSILON) {
        printf("  Skipped: the DP needs a whole-number capacity of at most %d.\n",
               MAX_DP_CAPACITY);
        return;
    }
    for (i = 0; i < packageCount; i++) {
        if (absDouble(packages[i].weight - (double)(long)(packages[i].weight + 0.5)) > EPSILON) {
            printf("  Skipped: the DP needs whole-number package weights.\n");
            return;
        }
    }
    capacity = (int)(vehicleCapacity[0] + 0.5);

    /* dpTable[i][c] = best value using the first i packages with capacity c */
    for (c = 0; c <= capacity; c++) {
        dpTable[0][c] = 0.0;
    }
    for (i = 1; i <= packageCount; i++) {
        int itemWeight = (int)(packages[i - 1].weight + 0.5);

        for (c = 0; c <= capacity; c++) {
            dpTable[i][c] = dpTable[i - 1][c];                 /* skip it   */
            if (itemWeight <= c
                && dpTable[i - 1][c - itemWeight] + packages[i - 1].value > dpTable[i][c]) {
                dpTable[i][c] = dpTable[i - 1][c - itemWeight] + packages[i - 1].value;
            }
        }
    }
    best = dpTable[packageCount][capacity];

    /* trace back which packages were chosen */
    c = capacity;
    for (i = packageCount; i >= 1; i--) {
        chosen[i - 1] = 0;
        if (dpTable[i][c] > dpTable[i - 1][c] + EPSILON) {
            chosen[i - 1] = 1;
            c -= (int)(packages[i - 1].weight + 0.5);
        }
    }

    printf("  Best 0/1 value (DP)          : %.2f\n", best);
    printf("  Whole packages chosen        :");
    for (i = 0; i < packageCount; i++) {
        if (chosen[i]) {
            printf(" %d(%s)", packages[i].number, packages[i].name);
            chosenWeight += packages[i].weight;
            anyChosen = 1;
        }
    }
    if (!anyChosen) {
        printf(" none");
    }
    printf("\n  Weight used (0/1)            : %.2f / %.2f\n", chosenWeight, vehicleCapacity[0]);
    printf("  Fractional greedy value      : %.2f\n", maxValue);
    printf("  Gain from allowing fractions : %.2f\n", maxValue - best);
    printf("  Greedy-by-ratio is optimal for the fractional problem, but 0/1 needs DP\n");
    printf("  (O(n*W)) because taking the best ratio first can waste capacity.\n");
}

/* ===========================================================================
 *  OPTION 5 : FIND MAXIMUM VALUE
 * ========================================================================= */

static void printTimeComplexity(void)
{
    printf("\nTIME COMPLEXITY\n");
    printf("  Ratio calculation ............ O(n)\n");
    if (sortMethodUsed == 2) {
        printf("  Merge sort by ratio .......... O(n log n)  <-- dominant step\n");
    } else {
        printf("  Bubble sort by ratio ......... O(n^2)      <-- dominant step\n");
    }
    printf("  Greedy selection scan ........ O(n + k)  (k = number of vehicles)\n");
    if (sortMethodUsed == 2) {
        printf("  Overall ...................... O(n log n)\n");
        printf("  Justification: merge sort halves the list log n times and merges n\n");
        printf("  elements per level, which outweighs the linear scans.\n");
    } else {
        printf("  Overall ...................... O(n^2)\n");
        printf("  Justification: the nested loops of bubble sort need about n(n-1)/2\n");
        printf("  comparisons, which outweighs the linear scans. Merge sort would\n");
        printf("  improve the total to O(n log n).\n");
    }
    printf("  Optional 0/1 comparison (DP) . O(n*W)\n");
    printf("  Space complexity: O(n).\n");
}

static void findMaximumValue(void)
{
    int stepMode;
    int v;

    if (!requireData()) {
        return;
    }
    if (!ratiosCalculated) {
        printf("\n(Ratios were not calculated yet - calculating them now.)");
        calculateRatio();
    }
    if (!sortedByRatio) {
        printf("\n(Packages were not sorted yet - sorting them now with bubble sort.)\n");
        sortPackages(1);
        printSortStats("Bubble sort", &lastSortStats);
        displayPackages("Packages in DECREASING order of Value/Weight ratio:");
    }

    stepMode = askYesNo("\nPause after each greedy step? (y/n): ");
    fractionalKnapsack(stepMode);

    if (vehicleCount > 1) {
        printf("\nVehicle usage:\n");
        for (v = 0; v < vehicleCount; v++) {
            printf("  Vehicle %d: weight %.2f / %.2f, value %.2f\n", v + 1,
                   vehicleWeight[v], vehicleCapacity[v], vehicleValue[v]);
        }
    }
    printf("\nTotal weight used : %.2f / %.2f\n", totalWeightUsed, totalCapacity());
    printf("*** MAXIMUM VALUE OBTAINED = %.2f ***\n", maxValue);

    compareWith01Knapsack();
    printTimeComplexity();
}

/* ===========================================================================
 *  OPTION 6 : DISPLAY SELECTED PACKAGES  (+ ASCII load bars, report export)
 * ========================================================================= */

/* One distinct character per package for the load bar */
static char symbolFor(int packageNumber)
{
    static const char symbols[] =
        "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    if (packageNumber >= 1 && packageNumber <= (int)(sizeof symbols) - 1) {
        return symbols[packageNumber - 1];
    }
    return '#';
}

/* [3333311111444.........]  each symbol = a package, '.' = empty space */
static void printLoadBar(FILE *out, int v)
{
    char   bar[BAR_WIDTH + 1];
    double cumulative = 0.0;
    int    position   = 0;
    int    end;
    int    i;

    memset(bar, '.', BAR_WIDTH);
    bar[BAR_WIDTH] = '\0';

    for (i = 0; i < packageCount; i++) {
        double weightHere = packages[i].vehicleFraction[v] * packages[i].weight;

        if (weightHere <= EPSILON) {
            continue;
        }
        cumulative += weightHere;
        end = (int)(cumulative / vehicleCapacity[v] * BAR_WIDTH + 0.5);
        if (end > BAR_WIDTH) {
            end = BAR_WIDTH;
        }
        while (position < end) {
            bar[position++] = symbolFor(packages[i].number);
        }
    }
    fprintf(out, "  Load   : [%s] %.0f%% full\n", bar,
            vehicleWeight[v] / vehicleCapacity[v] * 100.0);
    fprintf(out, "  Legend :");
    for (i = 0; i < packageCount; i++) {
        if (packages[i].vehicleFraction[v] > EPSILON) {
            fprintf(out, " %c=%s", symbolFor(packages[i].number), packages[i].name);
        }
    }
    fprintf(out, " ('.' = empty)\n");
}

/* The full selected-packages report, written to the screen or to a file */
static void printReport(FILE *out)
{
    int i;
    int v;
    double valueSum  = 0.0;
    double weightSum = 0.0;

    fprintf(out, "\nSELECTED PACKAGES (in decreasing ratio order)\n");
    printVehicleInfo(out);
    fprintf(out, "+---------+------------------+----------+--------------+--------------+-------------+\n");
    fprintf(out, "| Package | Name             | Fraction | Type         | Value gained | Weight used |\n");
    fprintf(out, "+---------+------------------+----------+--------------+--------------+-------------+\n");
    for (i = 0; i < packageCount; i++) {
        const char *type;
        double valueGained = packages[i].fraction * packages[i].value;
        double weightHere  = packages[i].fraction * packages[i].weight;

        if (packages[i].fraction <= EPSILON) {
            type = "Not selected";
        } else if (packages[i].fraction >= 1.0 - EPSILON) {
            type = "Complete";
        } else {
            type = "Fractional";
        }
        fprintf(out, "| %7d | %-16.16s | %8.4f | %-12s | %12.2f | %11.2f |\n",
                packages[i].number, packages[i].name, packages[i].fraction,
                type, valueGained, weightHere);
        valueSum  += valueGained;
        weightSum += weightHere;
    }
    fprintf(out, "+---------+------------------+----------+--------------+--------------+-------------+\n");
    fprintf(out, "| %-7s | %-16s | %8s | %-12s | %12.2f | %11.2f |\n",
            "TOTAL", "", "", "", valueSum, weightSum);
    fprintf(out, "+---------+------------------+----------+--------------+--------------+-------------+\n");
    fprintf(out, "Total weight used = %.2f / %.2f | Maximum value = %.2f\n",
            weightSum, totalCapacity(), valueSum);

    fprintf(out, "\nVEHICLE LOADS\n");
    for (v = 0; v < vehicleCount; v++) {
        fprintf(out, "Vehicle %d: capacity %.2f | used %.2f | value %.2f\n", v + 1,
                vehicleCapacity[v], vehicleWeight[v], vehicleValue[v]);
        printLoadBar(out, v);
        if (vehicleCount > 1) {
            for (i = 0; i < packageCount; i++) {
                if (packages[i].vehicleFraction[v] > EPSILON) {
                    fprintf(out, "    Package %d (%s): %.4f of the package -> weight %.2f, value %.2f\n",
                            packages[i].number, packages[i].name,
                            packages[i].vehicleFraction[v],
                            packages[i].vehicleFraction[v] * packages[i].weight,
                            packages[i].vehicleFraction[v] * packages[i].value);
                }
            }
        }
    }

    if (sortMethodUsed != 0) {
        fprintf(out, "\nSorting used: %s - %ld comparisons, %ld %s\n",
                sortMethodUsed == 2 ? "merge sort" : "bubble sort",
                lastSortStats.comparisons, lastSortStats.moves,
                sortMethodUsed == 2 ? "element writes" : "swaps");
    }
    if (comparedBoth) {
        fprintf(out, "Bubble sort needed %ld comparisons; merge sort needed %ld.\n",
                bubbleStats.comparisons, mergeStats.comparisons);
    }
}

static void exportReport(void)
{
    char  filename[LINE_SIZE];
    FILE *fp;

    printf("File name for the report [%s]: ", DEFAULT_REPORT_FILE);
    readLine(filename, sizeof filename);
    if (filename[0] == '\0') {
        snprintf(filename, sizeof filename, "%s", DEFAULT_REPORT_FILE);
    }
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("[!] Could not open '%s' for writing.\n", filename);
        return;
    }
    fprintf(fp, "SMART DELIVERY PLANNING - FRACTIONAL KNAPSACK REPORT\n");
    printReport(fp);
    fprintf(fp, "\nTime complexity: O(n^2) with bubble sort, O(n log n) with merge sort "
                "(sorting dominates).\n");
    if (fclose(fp) != 0) {
        printf("[!] Error while writing '%s'.\n", filename);
        return;
    }
    printf("Report saved to '%s'.\n", filename);
}

static void displaySelected(void)
{
    if (!requireData()) {
        return;
    }
    if (!knapsackSolved) {
        printf("\n[!] Maximum value not calculated yet. Use option 5 first.\n");
        return;
    }
    printReport(stdout);
    if (askYesNo("\nSave this report to a text file? (y/n): ")) {
        exportReport();
    }
}

/* ===========================================================================
 *  MAIN MENU
 * ========================================================================= */

static void printMenu(void)
{
    printf("\n=========== SMART DELIVERY PLANNING - FRACTIONAL KNAPSACK ===========\n");
    printf(" 1. Enter Package Details\n");
    printf(" 2. Display Package Details\n");
    printf(" 3. Calculate Value/Weight Ratio\n");
    printf(" 4. Sort Packages by Ratio\n");
    printf(" 5. Find Maximum Value\n");
    printf(" 6. Display Selected Packages\n");
    printf(" 7. Exit\n");
    printf("======================================================================\n");
    printf("Enter your choice (1-7): ");
}

int main(void)
{
    int choice;

    for (;;) {
        printMenu();
        if (!readInt(&choice)) {
            exitOnEof();
            printf("\n[!] Invalid choice. Please enter a number from 1 to 7.\n");
            continue;
        }

        switch (choice) {
        case 1:
            enterPackages();
            break;

        case 2:
            if (requireData()) {
                displayPackages("PACKAGE DETAILS:");
            }
            break;

        case 3:
            if (requireData()) {
                calculateRatio();
                displayPackages("Package details with Value/Weight ratios:");
            }
            break;

        case 4:
            if (requireData()) {
                sortMenu();
            }
            break;

        case 5:
            findMaximumValue();
            break;

        case 6:
            displaySelected();
            break;

        case 7:
            printf("\nExiting program. Goodbye!\n");
            return 0;

        default:
            printf("\n[!] Invalid choice '%d'. Please enter a number from 1 to 7.\n",
                   choice);
            break;
        }
    }
    return 0;
}
