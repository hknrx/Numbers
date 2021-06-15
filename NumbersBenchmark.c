// Nicolas Robert [Nrx]

// C libraries
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>

// Numbers library
#include "NumbersLibrary.h"

// Benchmark data
static struct {
    uint32_t workerCount;
    uint32_t tileCount;
    enum {
        IMPOSSIBLE_TARGET,
        RANDOM_TARGET,
        ALL_TARGETS
    } targetType;
} benchmarkData;

// Worker data
typedef struct {
    uint32_t workerId;
    pthread_t thread;
    uint64_t durationTotal;
    uint64_t durationMin;
    uint64_t durationMax;
    uint32_t solverCallCount;
    uint32_t abortedCount;
    uint32_t errorCount;
    uint16_t complexityMax;
} WorkerData;

// Combination data
static struct {
    pthread_mutex_t mutex;
    pthread_cond_t generate;
    pthread_cond_t solve;
    uint32_t* tileValues;
    uint32_t tileValuesStop;
} combinationData;

// Get the time
static uint64_t TimeGet (void) {
    struct timeval t;
    gettimeofday (&t, NULL);
    return t.tv_sec * 1000000ULL + t.tv_usec;
}

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

// Generate all possible tile combinations
static void CombinationGenerateAll (void) {

    // Get the mutex
    pthread_mutex_lock (&combinationData.mutex);

    // List of tiles without the duplicates
    const uint32_t tileSet[14] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 50, 75, 100};

    // Tiles used for the game
    uint32_t tileValues[benchmarkData.tileCount];

    // Let's make all possible combinations of "tileCount" tiles
    puts ("Generate all possible tile combinations:");
    uint32_t combinationCount = 0;
    uint32_t pairCount = benchmarkData.tileCount >> 1;
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
            uint32_t tileGroup = (1 << (benchmarkData.tileCount - tilePairId)) - 1;
            do {
                if ((tileGroup & pairGroup) == 0) {

                    // Get the tiles
                    tileSetId = 0;
                    for (uint32_t tileId = tilePairId; tileId < benchmarkData.tileCount; ++tileId) {
                        while ((tileGroup & (1 << tileSetId)) == 0) {
                            ++tileSetId;
                        }
                        tileValues[tileId] = tileSet[tileSetId++];
                    }

                    // Wait for a worker thread to use this combination
                    combinationData.tileValues = tileValues;
                    pthread_cond_signal (&combinationData.solve);
                    pthread_cond_wait (&combinationData.generate, &combinationData.mutex);

                    // Display the progress
                    if (++combinationCount % 100 == 0) {
                        if (combinationCount % 1000 == 0) {
                            printf ("%u\n", combinationCount);
                        } else {
                            printf (".");
                            fflush (stdout);
                        }
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

    // Inform all worker threads that there is no more work
    combinationData.tileValues = &combinationData.tileValuesStop;
    pthread_cond_broadcast (&combinationData.solve);

    // Finalize display of the progress
    printf ("%s"
        "(%u tile%s => %u combination%s)\n\n",
        combinationCount % 1000 < 100 ? "" : "\n",
        benchmarkData.tileCount, benchmarkData.tileCount > 1 ? "s" : "",
        combinationCount, combinationCount > 1 ? "s" : "");

    // Release the mutex
    pthread_mutex_unlock (&combinationData.mutex);
}

// Worker start routine: solve the game
static void* Worker (void* data) {

    // Initialize the worker
    WorkerData* workerData = (WorkerData*)data;
    workerData->durationTotal = 0;
    workerData->durationMin = UINT64_MAX;
    workerData->durationMax = 0;
    workerData->solverCallCount = 0;
    workerData->abortedCount = 0;
    workerData->errorCount = 0;
    workerData->complexityMax = 0;

    // Initialize the solver
    NumbersObject numbersObject = NULL;
    NumbersError error = NumbersInitialize (25000, 15000, &numbersObject);
    if (error != Success) {
        puts ("Error: Could not initialize the solver.");
        pthread_exit ((void*)-1);
    }

    // Initialize the PRNG
    RandomObject randomObject = RandomInitialize ();
    RandomSetSeed (randomObject, time (NULL));

    // Tiles used for the game
    uint32_t tileValues[benchmarkData.tileCount];
    NumbersTiles tiles = {benchmarkData.tileCount, tileValues};

    // Define the solution (even though it will not be displayed, it matters
    // for the benchmark)
    uint8_t solutionOperations[benchmarkData.tileCount];

    // Work!
    while (1) {

        // Get a combination
        pthread_mutex_lock (&combinationData.mutex);
        while (!combinationData.tileValues) {
            pthread_cond_wait (&combinationData.solve, &combinationData.mutex);
        }
        if (combinationData.tileValues == &combinationData.tileValuesStop) {
            tiles.values = NULL;
        } else {
            memcpy (tileValues, combinationData.tileValues, sizeof (tileValues));
            combinationData.tileValues = NULL;
            pthread_cond_signal (&combinationData.generate);
        }
        pthread_mutex_unlock (&combinationData.mutex);
        if (!tiles.values) {
            break;
        }

        // Set a target for this combination
        uint32_t target;
        if (benchmarkData.targetType == ALL_TARGETS) {
            target = 101;
        } else if (benchmarkData.targetType == RANDOM_TARGET) {
            target = 101 + (RandomGetValue (randomObject) % 899);
        } else {
            target = UINT32_MAX;
        }
        while (1) {

            // Define the complexity
            uint16_t complexity = UINT16_MAX;

            // Solve the game
            uint64_t duration = TimeGet ();
            error = NumbersSolve (numbersObject, target, &tiles, &complexity, solutionOperations, NULL);
            duration = TimeGet () - duration;

            // Record data
            workerData->durationTotal += duration;
            if (workerData->durationMin > duration) {
                workerData->durationMin = duration;
            }
            if (workerData->durationMax < duration) {
                workerData->durationMax = duration;
            }
            ++workerData->solverCallCount;
            if (error == AbortedError) {
                ++workerData->abortedCount;
            } else if (error != Success) {
                ++workerData->errorCount;
            }
            if (workerData->complexityMax < complexity) {
                workerData->complexityMax = complexity;
            }

            // Next target (if applicable)
            if (benchmarkData.targetType != ALL_TARGETS || target >= 999) {
                break;
            }
            ++target;
        }
    }

    // Shut down the PRNG
    RandomShutdown (randomObject);

    // Shut down the solver
    NumbersShutdown (numbersObject);

    // Done
    pthread_exit (NULL);
}

// Display the usage
static void UsageDisplay (char* name) {
    printf ("Usage:\n"
        "%s [<thread count (1-32)> [<tile count (0-8)> [impossible | random | all]]]\n",
        name);
}

// Check the arguments
static int ArgumentsCheck (int argc, char** argv) {

    // Check the first argument
    if (argc <= 1) {
        return 0;
    }
    char* argEnd = NULL;
    benchmarkData.workerCount = StringToNumber (argv[1], &argEnd);
    if (*argEnd != '\0' || benchmarkData.workerCount < 1 || benchmarkData.workerCount > 32) {
        return -1;
    }

    // Check the second argument
    if (argc <= 2) {
        return 0;
    }
    benchmarkData.tileCount = StringToNumber (argv[2], &argEnd);
    if (*argEnd != '\0' || benchmarkData.tileCount > 8) {
        return -1;
    }

    // Check the third argument
    if (argc <= 3) {
        return 0;
    }
    if (!strcmp (argv[3], "impossible")) {
        benchmarkData.targetType = IMPOSSIBLE_TARGET;
    } else if (!strcmp (argv[3], "random")) {
        benchmarkData.targetType = RANDOM_TARGET;
    } else if (!strcmp (argv[3], "all")) {
        benchmarkData.targetType = ALL_TARGETS;
    } else {
        return -1;
    }

    // Check whether there is a fourth argument
    if (argc <= 4) {
        return 0;
    }
    return -1;
}

// Main
int main (int argc, char** argv) {

    // Get the time
    uint64_t durationReal = TimeGet ();

    // Check the arguments
    benchmarkData.workerCount = 4;
    benchmarkData.tileCount = 6;
    benchmarkData.targetType = IMPOSSIBLE_TARGET;
    if (ArgumentsCheck (argc, argv)) {
        UsageDisplay (argv[0]);
        return -1;
    }

    // Initialize the combination data
    int threadError = pthread_mutex_init (&combinationData.mutex, NULL);
    threadError |= pthread_cond_init (&combinationData.generate, NULL);
    threadError |= pthread_cond_init (&combinationData.solve, NULL);
    if (threadError) {
        puts ("Error: Could not initialize a mutex or condition variable.");
        return -1;
    }
    combinationData.tileValues = NULL;

    // Create some worker threads
    WorkerData workersData[benchmarkData.workerCount];
    for (uint32_t workerId = 0; workerId < benchmarkData.workerCount; ++workerId) {
        WorkerData* workerData = &workersData[workerId];
        workerData->workerId = workerId;
        threadError = pthread_create (&workerData->thread, NULL, Worker, (void*)workerData);
        if (threadError) {
            puts ("Error: Could not create a worker thread.");
        }
    }

    // Generate all possible tile combinations
    CombinationGenerateAll ();

    // Wait for all the worker threads to complete
    uint64_t durationTotal = 0;
    uint64_t durationMin = UINT64_MAX;
    uint64_t durationMax = 0;
    uint32_t solverCallCount = 0;
    uint32_t abortedCount = 0;
    uint32_t errorCount = 0;
    uint16_t complexityMax = 0;
    for (uint32_t workerId = 0; workerId < benchmarkData.workerCount; ++workerId) {
        WorkerData* workerData = &workersData[workerId];
        void* workerStatus = NULL;
        threadError = pthread_join (workerData->thread, &workerStatus);
        if (threadError) {
            puts ("Error: Could not join with a worker thread.");
            continue;
        }
        if (workerStatus != NULL) {
            puts ("Error: A worker thread has failed.");
            continue;
        }

        // Merge the results
        durationTotal += workerData->durationTotal;
        if (durationMin > workerData->durationMin) {
            durationMin = workerData->durationMin;
        }
        if (durationMax < workerData->durationMax) {
            durationMax = workerData->durationMax;
        }
        solverCallCount += workerData->solverCallCount;
        abortedCount += workerData->abortedCount;
        errorCount += workerData->errorCount;
        if (complexityMax < workerData->complexityMax) {
            complexityMax = workerData->complexityMax;
        }
    }

    // Destroy the combination data
    pthread_cond_destroy (&combinationData.solve);
    pthread_cond_destroy (&combinationData.generate);
    pthread_mutex_destroy (&combinationData.mutex);

    // Determine whether the solver supports complexity check or not
    uint16_t complexityCheck = UINT16_MAX;
    NumbersObject numbersObject = NULL;
    NumbersError numbersError = NumbersInitialize (4, 1, &numbersObject);
    if (numbersError == Success) {
        uint32_t tileValues[] = {2, 2};
        NumbersTiles tiles = {2, tileValues};
        numbersError = NumbersSolve (numbersObject, 4, &tiles, &complexityCheck, NULL, NULL);
        NumbersShutdown (numbersObject);
    }

    // Get the time
    durationReal = TimeGet () - durationReal;

    // Display the results
    printf (
        "Solver called %u time%s (%u aborted call%s & %u error%s).\n",
        solverCallCount, solverCallCount > 1 ? "s" : "",
        abortedCount, abortedCount > 1 ? "s" : "",
        errorCount, errorCount > 1 ? "s" : "");
    if (numbersError != Success) {
        puts ("Error: Could not determine whether \"NumbersLibrary\" supports complexity check or not.");
    } else {
        printf ("The solver %s complexity check", complexityCheck ? "supports" : "does NOT support");
        if (benchmarkData.targetType != IMPOSSIBLE_TARGET) {
            printf (" (max. complexity: %hu)", complexityMax);
        }
        puts (".");
    }
    printf (
        "Test duration: %.3f s (i.e. an average of %" PRIu64 " us per %s with %u thread%s, including the test structure overhead).\n"
        "Average duration to %s: %" PRIu64 " us (min.: %" PRIu64 " us, max.: %" PRIu64 " us).\n",
        durationReal / 1000000.0f,
        solverCallCount ? durationReal / solverCallCount : 0,
        benchmarkData.targetType != IMPOSSIBLE_TARGET ? "game" : "combination",
        benchmarkData.workerCount, benchmarkData.workerCount > 1 ? "s" : "",
        benchmarkData.targetType != IMPOSSIBLE_TARGET ? "solve the game" : "analyze all solutions of a combination",
        solverCallCount ? durationTotal / solverCallCount : 0,
        solverCallCount ? durationMin : 0,
        durationMax);

    // Done
    pthread_exit (NULL);
}
