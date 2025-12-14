#include "ParallelBruteForce.h"
#include "FirstLetterPartitionGenerator.h"
#include "PipeClient.h"

std::vector<std::vector<char>> PartitionAlphabet(const std::string& alphabet, int threadCount) {
    std::vector<std::vector<char>> partitions;

    int charsPerThread = alphabet.length() / threadCount;
    int remainder = alphabet.length() % threadCount;

    int startIdx = 0;
    for (int i = 0; i < threadCount; i++) {
        // First 'remainder' threads get +1 char for balance
        int charsForThisThread = charsPerThread + (i < remainder ? 1 : 0);

        std::vector<char> partition;
        for (int j = 0; j < charsForThisThread; j++) {
            partition.push_back(alphabet[startIdx + j]);
        }

        partitions.push_back(partition);
        startIdx += charsForThisThread;
    }

    return partitions;
}

void BruteForceWorker(int threadId,
                     const std::string& alphabet,
                     int maxLength,
                     const std::vector<char>& myFirstLetters,
                     AttackContext* context)
{
    PipeClient client;

    // Create partition generator for my assigned first letters
    FirstLetterPartitionGenerator generator(alphabet, maxLength, myFirstLetters);

    // Test passwords until found or exhausted
    // Note: TryPassword() handles its own connect/disconnect cycle per requirements
    while (generator.HasNext() && !context->IsFound()) {
        std::string password = generator.Next();

        context->IncrementAttempts();

        if (client.TryPassword(context->targetLogin, password)) {
            context->MarkFound(password);
            break;
        }
    }
}
