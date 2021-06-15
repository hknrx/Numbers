// Nicolas Robert [Nrx]

// C libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Numbers library
#include "NumbersLibrary.h"

// Random library
#include "RandomLibrary.h"

// Define the various tests
typedef enum {
    CUSTOM_TEST,
    UNIT_TEST,
    RANDOM_TEST,
    FULL_TEST,
    ERROR
} Test;

// Convert a string to a number
static uint32_t StringToNumber (char* string, char** end) {
    uint32_t value = 0;
    if (string) {
        while (*string >= '0' && *string <= '9') {
            value =value * 10 + (*string - '0');
            ++string;
        }
    }
    if (end) {
        *end = string;
    }
    return value;
}

// Operation hook
static void OperationHook (NumbersOperation* operation) {
    printf (" %u %c %u = %u\n",
        operation->valueLeft,
        operation->opChar,
        operation->valueRight,
        operation->valueResult);
}

// Solve a game
static NumbersError Solve (NumbersObject numbersObject, RandomObject randomObject, uint32_t target, NumbersTiles* tiles, uint16_t* complexity) {

    // Check the pointers
    if (!tiles || !complexity) {
        return NullPointerError;
    }

    // Show the target
    printf (">> %u <<\n\n", target);

    // Show all the tiles
    if (tiles->count) {
        for (uint32_t tileId = 0; tileId < tiles->count; ++tileId) {
            printf (" [%u]", tiles->values[tileId]);
        }
        puts ("\n");
    }

    // Shuffle the tiles (to get alternate solutions)
    NumbersShuffle (tiles, randomObject);

    // Define the solution
    uint8_t solutionOperations[tiles->count];

    // Solve the game
    clock_t duration = clock ();
    NumbersError errorSolve = NumbersSolve (numbersObject, target, tiles, complexity, solutionOperations, NULL);
    duration = clock () - duration;

    if (errorSolve != Success && errorSolve != AbortedError) {

        // Show the error
        printf ("Error: %d", errorSolve);
    } else {

        // Show the list of operations
        uint32_t bestResult;
        NumbersError errorValidate = NumbersValidate (target, tiles, solutionOperations, &bestResult, OperationHook);
        if (errorValidate != Success) {

            // Show the error
            printf ("\nError: %d", errorValidate);
        } else {
            if (solutionOperations[0]) {
                puts ("");
            }

            // Show the best result, the complexity of the solution, and the
            // status of the solver
            printf ("Best result: %u, complexity: %hu, status: %s",
                bestResult,
                *complexity,
                errorSolve == AbortedError ? "ABORTED" : "OK");
        }
    }

    // Show the duration
    printf (", duration: %.2f ms\n", 1000.0f * duration / CLOCKS_PER_SEC);
    return errorSolve;
}

// Custom test
static void CustomTest (NumbersObject numbersObject, RandomObject randomObject, uint32_t target, NumbersTiles* tiles) {

    // Define the complexity
    uint16_t complexity = UINT16_MAX;

    // Solve the game
    Solve (numbersObject, randomObject, target, tiles, &complexity);
}

// Unit test
static void UnitTest (NumbersObject numbersObject, RandomObject randomObject) {

    // Define the target
    uint32_t target = 899;

    // Define the set of tiles
    uint32_t tileValues[] = {1, 1, 4, 5, 6, 7};
    NumbersTiles tiles = {sizeof (tileValues) / sizeof (tileValues[0]), tileValues};

    // Define the complexity
    uint16_t complexity = UINT16_MAX;

    // Solve the game
    Solve (numbersObject, randomObject, target, &tiles, &complexity);
}

// Random test
static void RandomTest (NumbersObject numbersObject, RandomObject randomObject, uint32_t tileCount) {

    // Define the target
    uint32_t target = 101 + (RandomGetValue (randomObject) % 899);

    // Define the set of tiles
    uint32_t tileSet[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 25, 50, 75, 100};
    NumbersTiles tiles = {sizeof (tileSet) / sizeof (tileSet[0]), tileSet};
    NumbersShuffle (&tiles, randomObject);
    if (tiles.count > tileCount) {
        tiles.count = tileCount;
    }

    // Define the complexity
    uint16_t complexity = UINT16_MAX;

    // Solve the game
    while (1) {
        NumbersError error = Solve (numbersObject, randomObject, target, &tiles, &complexity);
        if ((error != Success && error != AbortedError) || complexity < 2) {
            break;
        }
        complexity -= 2;
        puts ("");
    }
}

// Full test
static void FullTest (NumbersObject numbersObject, RandomObject randomObject, uint32_t target_, uint32_t tileCount) {

    // Check the number of tiles
    if (tileCount > 7) {
        printf ("Error: %d\n", TooManyTilesError);
        return;
    }

    // List of tiles without the duplicates
    const uint32_t tileSet[14] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 50, 75, 100};

    // Tiles used for the game
    uint32_t tileValues[tileCount];
    NumbersTiles tiles = {tileCount, tileValues};

    // Define the solution
    uint8_t solutionOperations[tileCount];

    // Let's make all possible combinations of "tileCount" tiles
    clock_t startTime = clock ();
    clock_t duration = 0;
    uint32_t combinationCount = 0;
    uint32_t abortedCount = 0;
    uint32_t errorCount = 0;
    uint16_t complexityMax = 0;
    uint32_t pairCount = tileCount >> 1;
    do {
        uint32_t pairGroup = (1 << pairCount) - 1;
        do {

            // Get the paired tiles
            uint32_t tileSetId = 0;
            uint32_t tilePairId = pairCount << 1;
            for (uint32_t tileId = 0; tileId < tilePairId; tileId += 2) {
                while ((pairGroup & (1 << tileSetId)) == 0) {
                    ++tileSetId;
                }
                uint32_t tileValue = tileSet[tileSetId++];
                tileValues[tileId] = tileValue;
                tileValues[tileId + 1] = tileValue;
            }

            // Get the other tiles
            uint32_t tileGroup = (1 << (tileCount - tilePairId)) - 1;
            do {
                if ((tileGroup & pairGroup) == 0) {

                    // Get the tiles
                    tileSetId = 0;
                    for (uint32_t tileId = tilePairId; tileId < tileCount; ++tileId) {
                        while ((tileGroup & (1 << tileSetId)) == 0) {
                            ++tileSetId;
                        }
                        tileValues[tileId] = tileSet[tileSetId++];
                    }

                    // Define the target
                    uint32_t target = target_ ? target_ : 101 + (RandomGetValue (randomObject) % 899);

                    // Define the complexity
                    uint16_t complexity = UINT16_MAX;

                    // Solve the game
                    duration -= clock ();
                    NumbersError error = NumbersSolve (numbersObject, target, &tiles, &complexity, solutionOperations, NULL);
                    duration += clock ();
                    if (++combinationCount % 100 == 0) {
                        if (combinationCount % 1000 == 0) {
                            printf ("%u\n", combinationCount);
                        } else {
                            printf (".");
                            fflush (stdout);
                        }
                    }
                    if (error == AbortedError) {
                        ++abortedCount;
                    } else if (error != Success) {
                        ++errorCount;
                    }
                    if (complexityMax < complexity) {
                        complexityMax = complexity;
                    }
                }

                // Next group of tiles
                if (tileGroup == 0) {
                    break;
                }
                uint32_t u = tileGroup & -tileGroup;
                uint32_t v = u + tileGroup;
                tileGroup = v + (((v ^ tileGroup) / u) >> 2);
            } while (tileGroup < (1 << 14));

            // Next group of pairs
            if (pairGroup == 0) {
                break;
            }
            uint32_t uu = pairGroup & -pairGroup;
            uint32_t vv = uu + pairGroup;
            pairGroup = vv + (((vv ^ pairGroup) / uu) >> 2);
        } while (pairGroup < (1 << 10));
    } while (pairCount--);

    // Display the results
    printf ("%s"
        "Total test duration: %.3f s\n"
        "%u tile%s => %u combination%s\n"
        "Actual solving duration: %.3f s (%u aborted & %u error%s)\n"
        "Average duration to solve the game: %.2f ms\n",
        combinationCount < 100 ? "" : (combinationCount % 1000 < 100 ? "\n" : "\n\n"),
        (float)(clock () - startTime) / CLOCKS_PER_SEC,
        tileCount, tileCount > 1 ? "s" : "",
        combinationCount, combinationCount > 1 ? "s" : "",
        (float)duration / CLOCKS_PER_SEC,
        abortedCount,
        errorCount, errorCount > 1 ? "s" : "",
        combinationCount ? 1000.0f * duration / CLOCKS_PER_SEC / combinationCount : 0.0f);
    if (target_ != UINT32_MAX) {
        printf ("Maximum complexity: %hu\n", complexityMax);
    }
}

// Display the usage
static void UsageDisplay (char* name) {
    printf ("Usage:\n"
        "%s -custom <target> <tile1> <tile2> <tile3> ...\n"
        "%s -unit\n"
        "%s -random [<tile count>]\n"
        "%s -full [<tile count> [<target> | impossible]]\n",
        name, name, name, name);
}

// Check the arguments
static Test ArgumentsCheck (int argc, char** argv, uint32_t* target, NumbersTiles* tiles) {

    // Check the first argument
    if (argc < 2 || argv[1][0] != '-') {
        return ERROR;
    }

    // Check the pointers
    if (!target || !tiles) {
        return ERROR;
    }

    // Custom test?
    if (!strcmp (argv[1], "-custom")) {
        tiles->count = argc > 3 ? argc - 3 : 0;
        tiles->values = malloc (sizeof (tiles->values[0]) * tiles->count);
        if (!tiles->values) {
            tiles->count = 0;
        } else {
            uint32_t* value = target;
            for (uint32_t argId = 2; argId < argc; ++argId) {
                char* argEnd = NULL;
                *value = StringToNumber (argv[argId], &argEnd);
                if (*argEnd != '\0') {
                    return ERROR;
                }
                value = &tiles->values[argId - 2];
            }
        }
        return CUSTOM_TEST;
    }

    // Unit test?
    if (!strcmp (argv[1], "-unit")) {
        if (argc != 2) {
            return ERROR;
        }
        return UNIT_TEST;
    }

    // Random test?
    if (!strcmp (argv[1], "-random")) {
        if (argc > 3) {
            return ERROR;
        }
        if (argc == 2) {
            tiles->count = 6;
        } else {
            char* argEnd = NULL;
            tiles->count = StringToNumber (argv[2], &argEnd);
            if (*argEnd != '\0') {
                return ERROR;
            }
        }
        return RANDOM_TEST;
    }

    // Full test?
    if (!strcmp (argv[1], "-full")) {
        if (argc > 4) {
            return ERROR;
        }
        if (argc == 2) {
            tiles->count = 6;
            *target = 0;
        } else {
            char* argEnd = NULL;
            tiles->count = StringToNumber (argv[2], &argEnd);
            if (*argEnd != '\0') {
                return ERROR;
            }
            if (argc == 3) {
                *target = 0;
            } else if (!strcasecmp (argv[3], "impossible")) {
                *target = UINT32_MAX;
            } else {
                *target = StringToNumber (argv[3], &argEnd);
                if (*argEnd != '\0') {
                    return ERROR;
                }
            }
        }
        return FULL_TEST;
    }

    // Done
    return ERROR;
}

// Main
int main (int argc, char** argv) {

    // Check the arguments
    uint32_t target = 0;
    NumbersTiles tiles = {0, NULL};
    Test test = ArgumentsCheck (argc, argv, &target, &tiles);
    if (test == ERROR) {
        UsageDisplay (argv[0]);
        return -1;
    }

    // Initialize the solver
    NumbersObject numbersObject = NULL;
    NumbersError error = NumbersInitialize (25000, 15000, &numbersObject);
    if (error != Success) {
        puts ("Error: Could not initialize the solver.");
        return -1;
    }

    // Initialize the PRNG
    RandomObject randomObject = RandomInitialize ();
    RandomSetSeed (randomObject, time (NULL));

    // Run the test
    switch (test) {
        case CUSTOM_TEST:
            CustomTest (numbersObject, randomObject, target, &tiles);
            break;
        case UNIT_TEST:
            UnitTest (numbersObject, randomObject);
            break;
        case RANDOM_TEST:
            RandomTest (numbersObject, randomObject, tiles.count);
            break;
        case FULL_TEST:
            FullTest (numbersObject, randomObject, target, tiles.count);
            break;
        default:
            UsageDisplay (argv[0]);
            break;
    }

    // Shut down the PRNG
    RandomShutdown (randomObject);

    // Shut down the solver
    NumbersShutdown (numbersObject);

    // Destroy the tiles (if any)
    free (tiles.values);

    // Done
    return 0;
}
