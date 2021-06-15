// Nicolas Robert [Nrx]

// Include guard
#ifndef RANDOM_LIBRARY_H
#define RANDOM_LIBRARY_H

// C libraries
#include <stdint.h>

// Random library object
typedef struct RandomStruct* RandomObject;

/**
 * Initialize the library, allocating the memory required by the pseudo random
 * number generator object.
 * @return Pseudo random number generator object.
 */
extern RandomObject RandomInitialize (void);

/**
 * Shut down the library, freeing the memory allocated during its
 * initialization.
 * @param randomObject Pseudo random number generator to shut down.
 */
extern void RandomShutdown (RandomObject randomObject);

/**
 * Set the seed of the pseudo random number generator.
 * @param randomObject Pseudo random number generator which the seed has to be
 * set.
 * @param seed Seed of the pseudo random number generator.
 */
extern void RandomSetSeed (RandomObject randomObject, uint64_t seed);

/**
 * Get a pseudo random value.
 * @see <a href="http://java.sun.com/javase/6/docs/api/java/util/Random.html">J2SE "Random" implementation</a>
 * @param randomObject Pseudo random number generator to be used.
 * @return A 32 bit pseudo random integer.
 */
extern uint32_t RandomGetValue (RandomObject randomObject);

// Include guard
#endif // RANDOM_LIBRARY_H
