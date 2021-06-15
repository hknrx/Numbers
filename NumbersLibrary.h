// Nicolas Robert [Nrx]

// "Des chiffres et des lettres" ("Numbers and letters") is one of the oldest
// TV show running in the world. The game has a numbers round called "Le compte
// est bon" ("The total is right"); this C program presents a solver for this
// game, using a breadth-first search algorithm.
// Reference: https://en.wikipedia.org/wiki/Des_chiffres_et_des_lettres

// Include guard
#ifndef NUMBERS_LIBRARY_H
#define NUMBERS_LIBRARY_H

// C libraries
#include <stdint.h>

// Random library
#include "RandomLibrary.h"

// Numbers library object
typedef struct NumbersStruct* NumbersObject;

// Errors
typedef enum {
    Success = 0,
    AbortedError,
    ZeroSizeError,
    MemoryAllocationError,
    NullPointerError,
    TooManyTilesError,
    IncorrectTileIdError,
    TileUsedTwiceError,
    NegativeResultError,
    DivisionByZeroError,
    RemainderNotNullError
} NumbersError;

// Tiles
typedef struct {
    uint32_t count;
    uint32_t* values;
} NumbersTiles;

// Operations
typedef struct {
    uint32_t valueLeft;
    char opChar;
    uint32_t valueRight;
    uint32_t valueResult;
} NumbersOperation;

// Operation hook
typedef void (*NumbersOperationHook)(NumbersOperation* operation);

/**
 * Initialize the library, allocating the memory required to search for a
 * solution. This function must be called once before NumbersSolve can be used.
 * The most appropriate parameters when playing with 6 tiles are operationSize
 * = 25000 and operationIdSize = 15000.
 * @param operationSize Maximum number of operations that can be analyzed.
 * @param operationIdSize Size of the array used to find duplicate results.
 * @param numbersObject Numbers library object (out).
 * @return Success if the library could be initialized successfully.
 */
extern NumbersError NumbersInitialize (uint16_t operationSize, uint16_t operationIdSize, NumbersObject* numbersObject);

/**
 * Shut down the library, freeing the memory allocated during its
 * initialization. This function should be called once after the final call to
 * NumbersSolve.
 * @param numbersObject Numbers library object to shut down.
 * @return Success if the library could be shut down successfully.
 */
extern NumbersError NumbersShutdown (NumbersObject numbersObject);

/**
 * Solve the game.
 * @param numbersObject Numbers library object used to solve the game.
 * @param target Target number (i.e. number which the solver tries to reach).
 * @param tiles Set of tiles (i.e. values that the solver combines to attempt
 * reaching the target). It shall be shuffled prior to calling this function to
 * get alternative solutions. There shall not be more than 8 tiles.
 * @param complexity Maximum complexity of the solution (in) / actual
 * complexity of the solution (out). One can pass a NULL pointer or UINT16_MAX
 * to be sure to get the best solution.
 * @param solutionOperations Array which stores the solution. Each operation is
 * stored on 1 byte (bits 0-2 represent the index of the left tile, bits 3-5 the
 * index of the right tile, and bits 6-7 the operator); the array shall have
 * room for tiles->count bytes since N tiles can be combined into (N - 1)
 * operations, and the end of the solution is marked with byte 0.
 * @param result Number reached by the solver.
 * @return Success if the game could be solved successfully, AbortedError if
 * the number of operations to analyze was greater than the allocated memory
 * (forcing the solver to abort its search), NullPointerError if numbersObject
 * or tiles is a NULL pointer, or TooManyTilesError if there are too many tiles.
 */
extern NumbersError NumbersSolve (NumbersObject numbersObject, uint32_t target, NumbersTiles* tiles, uint16_t* complexity, uint8_t* solutionOperations, uint32_t* result);

/**
 * Shuffle a set of tiles. This allows to get different solutions for a given
 * problem (although NumbersSolve will always return the best solution
 * possible, order of operations to get to the result may differ).
 * @param tiles Set of tiles to be shuffled.
 * @param randomObject PRNG used to shuffle the tiles.
 * @return Success if the tiles could be shuffled successfully.
 */
extern NumbersError NumbersShuffle (NumbersTiles* tiles, RandomObject randomObject);

/**
 * Validate a solution. This function can be used both to decipher solutions
 * proposed by NumbersSolve, and to validate solutions proposed by other means
 * (e.g. human players, as long as they encode their operations in the same
 * way).
 * @param target Target number.
 * @param tiles Set of tiles (i.e. values that were combined to attempt
 * reaching the target).
 * @param solutionOperations Array which stores the solution. Each operation is
 * stored on 1 byte (bits 0-2 represent the index of the left tile, bits 3-5 the
 * index of the right tile, and bits 6-7 the operator); the end of the solution
 * is marked with byte 0.
 * @param bestResult Result (or tile) which the value is the closest to the
 * target number.
 * @param operationHook Function called at each step of the resolution (i.e.
 * once for each operation).
 * @return Success if the solution is valid.
 */
extern NumbersError NumbersValidate (uint32_t target, NumbersTiles* tiles, uint8_t* solutionOperations, uint32_t* bestResult, NumbersOperationHook operationHook);

// Include guard
#endif // NUMBERS_LIBRARY_H
