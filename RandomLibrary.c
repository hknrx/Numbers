// Nicolas Robert [Nrx]

// C libraries
#include <stdlib.h>

// Random library
#include "RandomLibrary.h"

// Library object
struct RandomStruct {
    uint64_t seed;
};

// Initialize the library
extern RandomObject RandomInitialize (void) {

    // Allocate an object
    RandomObject randomObject = malloc (sizeof (struct RandomStruct));

    // Return the object
    return randomObject;
}

// Shut down the library
extern void RandomShutdown (RandomObject randomObject) {

    // Destroy the object
    free (randomObject);
}

// Set the seed
extern void RandomSetSeed (RandomObject randomObject, uint64_t seed) {

    // Check the object
    if (randomObject) {

        // Set the seed of the pseudo random number generator
        randomObject->seed = (seed ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1);
    }
}

// Get a pseudo random value
extern uint32_t RandomGetValue (RandomObject randomObject) {

    // Check the object
    if (!randomObject) {
        return 0;
    }

    // Return a random value
    randomObject->seed = (randomObject->seed * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1);
    return (uint32_t)(randomObject->seed >> (48 - 32));
}
