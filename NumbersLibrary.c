// Nicolas Robert [Nrx]

// C libraries
#include <stdlib.h>

// Numbers library
#include "NumbersLibrary.h"

// Boolean
typedef enum {
    FALSE = 0,
    TRUE = 1
} Bool;

// Operators
typedef enum {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3,
    NOP
} Operator;

// Operations
typedef struct {
    uint16_t operationIdLeft;
    uint16_t operationIdRight;
    uint32_t result;
    uint8_t op;
#ifndef DISABLE_COMPLEXITY
    uint8_t weight;
    uint16_t complexity;
#endif
} Operation;

// Groups of operations
typedef struct {
    uint16_t operationIdFirst;
    uint16_t operationIdLast;
} OperationGroup;

// Library object
struct NumbersStruct {

    // Target number and count of tiles
    struct {
        uint32_t target;
        uint32_t tileCount;
    } final;

    // Solution
    struct {
        uint16_t operationId;
        uint32_t targetDiff;
        uint32_t tileCount;
        uint8_t* operations;
        Bool aborted;
    } solution;

    // Current values
    struct {
        uint32_t tileCount;
#ifndef DISABLE_COMPLEXITY
        uint16_t complexityMax;
#endif
        uint16_t operationIdFirst;
        uint16_t operationIdLast;
        uint16_t operationIdLeft;
        uint16_t operationIdRight;
    } current;

    // Operation IDs (to search for an existing result in the last operation
    // group)
    uint16_t operationIdSize;
    uint16_t* operationIds;

    // Operations
    uint16_t operationSize;
    Operation operations[];
};

// Initialize the library
extern NumbersError NumbersInitialize (uint16_t operationSize, uint16_t operationIdSize, NumbersObject* numbersObject_) {

    // Check the pointer
    if (!numbersObject_) {
        return NullPointerError;
    }
    *numbersObject_ = NULL;

    // Check the size parameters
    if (!operationSize || !operationIdSize) {
        return ZeroSizeError;
    }

    // Allocate an object (including an array to record all operations, and an
    // array to allow searching for a given result in a group of operations)
    NumbersObject numbersObject = malloc (sizeof (struct NumbersStruct)
        + sizeof (numbersObject->operations[0]) * operationSize
        + sizeof (numbersObject->operationIds[0]) * operationIdSize);
    if (!numbersObject) {
        return MemoryAllocationError;
    }

    // Initialize the object
    numbersObject->operationSize = operationSize;
    numbersObject->operationIdSize = operationIdSize;
    numbersObject->operationIds = (uint16_t*)&numbersObject->operations[operationSize];

    // Return the object
    *numbersObject_ = numbersObject;

    // Done
    return Success;
}

// Shut down the library
extern NumbersError NumbersShutdown (NumbersObject numbersObject) {

    // Check the pointer
    if (!numbersObject) {
        return NullPointerError;
    }

    // Destroy the object
    free (numbersObject);

    // Done
    return Success;
}

// Search for a given result in the current operation group
// Note: better not perform a linear search if the result is outside the array,
// this would be slower than to record and process the duplicate result!
inline static uint16_t SearchOperation (NumbersObject numbersObject, uint32_t result) {
    if (result < numbersObject->operationIdSize) {
        uint16_t operationId = numbersObject->operationIds[result];
        if (operationId >= numbersObject->current.operationIdFirst
            && operationId < numbersObject->current.operationIdLast
            && numbersObject->operations[operationId].result == result) {
            return operationId;
        }
    }
    return numbersObject->current.operationIdLast;
}

#ifndef DISABLE_COMPLEXITY
// Compute the weight of a result
static uint8_t ComputeResultWeight (uint32_t result) {
    uint8_t weight = 1;
    if (result > 100) {
        if (result > 1000) {
            weight = 7;
        } else if (result % 10) {
            weight = 5;
        } else if (result % 100) {
            weight = 3;
        }
    } else if (result > 10) {
        if (result % 10) {
            if (result != 25 && result != 75) {
                weight = 3;
            }
        } else if (result == 100) {
            weight = 0;
        }
    } else if (result == 1 || result == 10) {
        weight = 0;
    }
    return weight;
}
#endif

// Record an operation
static void RecordOperation (NumbersObject numbersObject, Operator op, uint32_t result, uint16_t complexity) {

#ifndef DISABLE_COMPLEXITY
    // Check the complexity
    if (complexity > numbersObject->current.complexityMax) {
        return;
    }
#endif

    // Compute the difference between this result and the target
    uint32_t targetDiff;
    if (result > numbersObject->final.target) {
        targetDiff = result - numbersObject->final.target;
    } else {
        targetDiff = numbersObject->final.target - result;
    }

    Bool bestResult = TRUE;
    uint16_t operationId;
    if (targetDiff < numbersObject->solution.targetDiff
        || numbersObject->solution.operationId >= numbersObject->operationSize) {

        // No doubt, this is the best result so far
        operationId = numbersObject->current.operationIdLast;
    } else if (numbersObject->current.tileCount == numbersObject->final.tileCount) {

        // Check whether this result is the best
        if (targetDiff > numbersObject->solution.targetDiff
#ifndef DISABLE_COMPLEXITY
            || complexity >= numbersObject->operations[numbersObject->solution.operationId].complexity
#endif
            || numbersObject->current.tileCount > numbersObject->solution.tileCount) {

            // No need to record an operation that will never be used
            return;
        }

        // Check whether this result has already been recorded in this group
        operationId = SearchOperation (numbersObject, result);
    } else {

        // Check whether this result has already been recorded in this group
        operationId = SearchOperation (numbersObject, result);
        if (operationId == numbersObject->current.operationIdLast) {

            // Check whether this result is the best
            bestResult = targetDiff == numbersObject->solution.targetDiff
#ifndef DISABLE_COMPLEXITY
                && complexity < numbersObject->operations[numbersObject->solution.operationId].complexity
#endif
                && numbersObject->current.tileCount == numbersObject->solution.tileCount ?
                TRUE : FALSE;
#ifndef DISABLE_COMPLEXITY
        } else if (complexity >= numbersObject->operations[operationId].complexity) {
#else
        } else {
#endif

            // An operation with the same result but a lower complexity is
            // already recorded...
            return;
        }
    }

    // Record or update the operation
    Operation* operation = &numbersObject->operations[operationId];
    if (operationId == numbersObject->current.operationIdLast) {

        // Make sure the operation can be recorded
        if (operationId >= numbersObject->operationSize) {

            // The array is full, let's return the best solution found so
            // far...
            numbersObject->solution.aborted = TRUE;
            return;
        }

        // If possible, record the ID of this operation, to allow searching for
        // its result
        if (result < numbersObject->operationIdSize) {
            numbersObject->operationIds[result] = operationId;
        }

        // Record the operation
        operation->result = result;
        ++numbersObject->current.operationIdLast;

        // Take note of the best solution so far
        if (bestResult) {
            numbersObject->solution.operationId = operationId;
            numbersObject->solution.targetDiff = targetDiff;
            numbersObject->solution.tileCount = numbersObject->current.tileCount;
        }

#ifndef DISABLE_COMPLEXITY
        // Take note of the "weight" of this result, to allow computing the
        // complexity of operations
        operation->weight = ComputeResultWeight (result);
#endif
    }
    operation->operationIdLeft = numbersObject->current.operationIdLeft;
    operation->operationIdRight = numbersObject->current.operationIdRight;
    operation->op = op;
#ifndef DISABLE_COMPLEXITY
    operation->complexity = complexity;
#endif
}

// Combine 2 groups of operations, computing all possible combinations of their
// results
static void CombineOperationGroups (NumbersObject numbersObject, OperationGroup* operationGroupA, OperationGroup* operationGroupB) {

    // Go through all results of the group A
    for (uint16_t operationIdA = operationGroupA->operationIdFirst;
        operationIdA < operationGroupA->operationIdLast; ++operationIdA) {

        // Get information about this result
        Operation* operationA = &numbersObject->operations[operationIdA];
        uint32_t resultA = operationA->result;
#ifndef DISABLE_COMPLEXITY
        uint8_t weightA = operationA->weight;
        uint16_t complexityA = operationA->complexity;
#endif

        // Go through all results of the group B
        for (uint16_t operationIdB = operationGroupB->operationIdFirst;
            operationIdB < operationGroupB->operationIdLast; ++operationIdB) {

            // Compute the overall complexity to get that far
            Operation* operationB = &numbersObject->operations[operationIdB];
#ifndef DISABLE_COMPLEXITY
            uint16_t complexityAB = complexityA + operationB->complexity;
            if (complexityAB > numbersObject->current.complexityMax) {
                continue;
            }
#endif

            // Get information about this result
            uint32_t resultB = operationB->result;
#ifndef DISABLE_COMPLEXITY
            uint8_t weightB = operationB->weight;
#endif

            // Order the results
            uint32_t resultMax;
            uint32_t resultMin;
            if (resultA >= resultB) {
                resultMax = resultA;
                resultMin = resultB;
                numbersObject->current.operationIdLeft = operationIdA;
                numbersObject->current.operationIdRight = operationIdB;
            } else {
                resultMax = resultB;
                resultMin = resultA;
                numbersObject->current.operationIdLeft = operationIdB;
                numbersObject->current.operationIdRight = operationIdA;
            }

            // Addition
#ifndef DISABLE_COMPLEXITY
            uint16_t complexity = complexityAB + (weightA < weightB ? weightA : weightB);
#else
            uint16_t complexity = 0;
#endif
            RecordOperation (numbersObject, ADD, resultMax + resultMin, complexity);

            // Subtraction
            if (resultMax != resultMin) {
                uint32_t result = resultMax - resultMin;
                if (result != resultMin) {
#ifndef DISABLE_COMPLEXITY
                    complexity = complexityAB + ((weightA + weightB) >> 1);
#endif
                    RecordOperation (numbersObject, SUB, result, complexity);
                }
            }

            // Make sure the smallest number is greater than 1
            if (resultMin > 1) {

                // Multiplication
                uint32_t result = resultMax * resultMin;
#ifndef DISABLE_COMPLEXITY
                complexity = weightA * weightB;
                complexity = complexityAB + complexity * complexity;
#endif
                RecordOperation (numbersObject, MUL, result, complexity);

                // Division
                if (resultMax == resultMin) {
#ifndef DISABLE_COMPLEXITY
                    complexity = complexityAB + 1;
#endif
                    RecordOperation (numbersObject, DIV, 1, complexity);
                } else {
                    result = resultMax / resultMin;
                    if (result != resultMin && result * resultMin == resultMax) {

                        // Note: when such a division is actually possible
                        // (which is not so frequent), it is probably not much
                        // harder to find than to perform the multiplication,
                        // hence the use of the same "complexity" value here
                        RecordOperation (numbersObject, DIV, result, complexity);
                    }
                }
            }
        }
    }
}

// Generate the solution
static uint16_t GenerateSolution (NumbersObject numbersObject, uint16_t operationId) {

    // Make sure there is an operation
    Operation* operation = &numbersObject->operations[operationId];
    if (operation->op == NOP) {
        return operationId;
    }

    // Track back...
    uint16_t tileIdLeft = GenerateSolution (numbersObject, operation->operationIdLeft);
    uint16_t tileIdRight = GenerateSolution (numbersObject, operation->operationIdRight);

    // Append the operation to the solution
    *numbersObject->solution.operations = tileIdLeft | (tileIdRight << 3) | (operation->op << 6);
    ++numbersObject->solution.operations;

    // Assume the result will be stored in the left tile
    return tileIdLeft;
}

// Solve the game
extern NumbersError NumbersSolve (NumbersObject numbersObject, uint32_t target, NumbersTiles* tiles, uint16_t* complexity, uint8_t* solutionOperations, uint32_t* result) {

    // Check the pointers
    if (!numbersObject || !tiles) {
        return NullPointerError;
    }
    if (tiles->count > 8) {
        return TooManyTilesError;
    }

    // Take note of the target and number of tiles
    numbersObject->final.target = target;
    numbersObject->final.tileCount = tiles->count;

    // Initialize the solution
    numbersObject->solution.operationId = numbersObject->operationSize;
    numbersObject->solution.aborted = FALSE;

    // Create an array to record groups of operations (1 group per combination
    // of tiles)
    uint16_t operationGroupSize = 1 << numbersObject->final.tileCount;
    OperationGroup operationGroups[operationGroupSize];

    // Record all the tiles
    numbersObject->current.tileCount = 1;
#ifndef DISABLE_COMPLEXITY
    numbersObject->current.complexityMax = complexity ? *complexity : UINT16_MAX;
#endif
    numbersObject->current.operationIdLast = 0;
    for (uint32_t tileId = 0; tileId < numbersObject->final.tileCount; ++tileId) {

        // Record the tile
        numbersObject->current.operationIdFirst = numbersObject->current.operationIdLast;
        RecordOperation (numbersObject, NOP, tiles->values[tileId], 0);

        // This tile alone defines a new operation group
        uint32_t tileGroup = 1 << tileId;
        operationGroups[tileGroup].operationIdFirst = numbersObject->current.operationIdFirst;
        operationGroups[tileGroup].operationIdLast = numbersObject->current.operationIdLast;
    }

    // Perform all possible tile combinations, starting with just 2 tiles then
    // adding some more
    while (numbersObject->current.tileCount < numbersObject->final.tileCount
        && numbersObject->solution.targetDiff != 0
        && !numbersObject->solution.aborted) {

        // Increase the number of tiles to include in the combination
        ++numbersObject->current.tileCount;

        // Define a group with the number of tiles specified
        uint32_t tileGroup = (1 << numbersObject->current.tileCount) - 1;
        do {

            // Break this group of tiles into 2 smaller groups (all possible
            // combinations)
            uint32_t tileSubGroupCount = (1 << (numbersObject->current.tileCount - 1)) - 1;
            uint32_t tileSubGroups[tileSubGroupCount];
            uint32_t tilesRemaining = tileGroup;
            for (uint32_t tileSubGroupId = 0; tileSubGroupId < tileSubGroupCount; ++tileSubGroupId) {
                tileSubGroups[tileSubGroupId] = 0;
            }
            for (uint32_t tileSubGroupMask = 1; tileSubGroupMask <= tileSubGroupCount; tileSubGroupMask <<= 1) {
                uint32_t tile = tilesRemaining & -tilesRemaining;
                for (uint32_t tileSubGroupNumber = 1; tileSubGroupNumber <= tileSubGroupCount; ++tileSubGroupNumber) {
                    if ((tileSubGroupNumber & tileSubGroupMask) != 0) {
                        tileSubGroups[tileSubGroupNumber - 1] += tile;
                    }
                }
                tilesRemaining -= tile;
            }

            // Combine all pairs of smaller groups
            numbersObject->current.operationIdFirst = numbersObject->current.operationIdLast;
            for (uint32_t tileSubGroupId = 0; tileSubGroupId < tileSubGroupCount && !numbersObject->solution.aborted; ++tileSubGroupId) {
                uint32_t tileSubGroup = tileSubGroups[tileSubGroupId];
                CombineOperationGroups (numbersObject, &operationGroups[tileSubGroup], &operationGroups[tileGroup - tileSubGroup]);
            }

            // Record this new operation group
            operationGroups[tileGroup].operationIdFirst = numbersObject->current.operationIdFirst;
            operationGroups[tileGroup].operationIdLast = numbersObject->current.operationIdLast;

            // Next group of tiles (with the same number of tiles)
            uint32_t u = tileGroup & -tileGroup;
            uint32_t v = u + tileGroup;
            tileGroup = v + (((v ^ tileGroup) / u) >> 2);
        } while (tileGroup < operationGroupSize && !numbersObject->solution.aborted);
    }

    // Make sure a solution has been found
    if (numbersObject->solution.operationId >= numbersObject->operationSize) {
        if (solutionOperations) {
            *solutionOperations = 0;
        }
        if (complexity) {
            *complexity = 0;
        }
        if (result) {
            *result = 0;
        }
    } else {

        // Generate the solution
        if (solutionOperations) {
            numbersObject->solution.operations = solutionOperations;
            GenerateSolution (numbersObject, numbersObject->solution.operationId);
            *numbersObject->solution.operations = 0;
        }
        if (complexity) {
#ifndef DISABLE_COMPLEXITY
            *complexity = numbersObject->operations[numbersObject->solution.operationId].complexity;
#else
            *complexity = 0;
#endif
        }
        if (result) {
            *result = numbersObject->operations[numbersObject->solution.operationId].result;
        }
    }
    return numbersObject->solution.aborted ? AbortedError : Success;
}

// Shuffle a set of tiles
extern NumbersError NumbersShuffle (NumbersTiles* tiles, RandomObject randomObject) {

    // Check the pointers
    if (!tiles || !randomObject) {
        return NullPointerError;
    }

    // Shuffle the tiles
    uint32_t tileId = tiles->count;
    while (tileId > 1) {
        uint32_t tileSwapId = RandomGetValue (randomObject) % tileId;
        --tileId;
        if (tileId != tileSwapId) {
            uint32_t tileValue = tiles->values[tileSwapId];
            tiles->values[tileSwapId] = tiles->values[tileId];
            tiles->values[tileId] = tileValue;
        }
    }

    // Done
    return Success;
}

// Check whether a result is the closest to the target number
inline static void CheckResult (uint32_t target, uint32_t result, uint32_t* bestResult, uint32_t* bestTargetDiff) {

    // Check whether this is the best result so far
    uint32_t targetDiff = result > target ? result - target : target - result;
    if (targetDiff < *bestTargetDiff) {
        *bestTargetDiff = targetDiff;
        *bestResult = result;
    }
}

// Validate a solution
extern NumbersError NumbersValidate (uint32_t target, NumbersTiles* tiles, uint8_t* solutionOperations, uint32_t* bestResult, NumbersOperationHook operationHook) {

    // Check the pointers
    if (!tiles || !bestResult) {
        return NullPointerError;
    }

    // Copy and check all the tiles
    *bestResult = 0;
    uint32_t bestTargetDiff = UINT32_MAX;
    uint32_t tileValues[tiles->count];
    for (uint32_t tileId = 0; tileId < tiles->count; ++tileId) {

        // Copy the tile
        uint32_t tileValue = tiles->values[tileId];
        tileValues[tileId] = tileValue;

        // Check whether this is the best tile so far
        CheckResult (target, tileValue, bestResult, &bestTargetDiff);
    }

    // Check each operation
    if (solutionOperations) {
        while (*solutionOperations) {

            // Get the tile IDs and operator
            uint8_t operationEncoded = *solutionOperations;
            uint8_t tileIdLeft = operationEncoded & 7;
            Operator op = (Operator)(operationEncoded >> 6);
            uint8_t tileIdRight = (operationEncoded >> 3) & 7;

            // Make sure the tile IDs are valid
            if (tileIdLeft >= tiles->count || tileIdRight >= tiles->count || tileIdLeft == tileIdRight) {
                return IncorrectTileIdError;
            }

            // Get the value of both tiles used in the operation
            NumbersOperation operation;
            operation.valueLeft = tileValues[tileIdLeft];
            operation.valueRight = tileValues[tileIdRight];

            // Make sure these tiles have not been used before
            if (operation.valueLeft == UINT32_MAX || operation.valueRight == UINT32_MAX) {
                return TileUsedTwiceError;
            }

            // Compute the result of the operation
            switch (op) {
                case ADD:
                    operation.valueResult = operation.valueLeft + operation.valueRight;
                    operation.opChar = '+';
                    break;
                case MUL:
                    operation.valueResult = operation.valueLeft * operation.valueRight;
                    operation.opChar = 'x';
                    break;
                case SUB:
                    if (operation.valueLeft < operation.valueRight) {
                        return NegativeResultError;
                    }
                    operation.valueResult = operation.valueLeft - operation.valueRight;
                    operation.opChar = '-';
                    break;
                default:
                    if (operation.valueRight == 0) {
                        return DivisionByZeroError;
                    }
                    operation.valueResult = operation.valueLeft / operation.valueRight;
                    if (operation.valueRight * operation.valueResult != operation.valueLeft) {
                        return RemainderNotNullError;
                    }
                    operation.opChar = '/';
                    break;
            }

            // Call the hook
            if (operationHook) {
                operationHook (&operation);
            }

            // Save the result in the left tile, and invalidate the right tile
            tileValues[tileIdLeft] = operation.valueResult;
            tileValues[tileIdRight] = UINT32_MAX;

            // Check whether this is the best result so far
            CheckResult (target, operation.valueResult, bestResult, &bestTargetDiff);

            // Next operation
            ++solutionOperations;
        }
    }

    // Done
    return Success;
}
