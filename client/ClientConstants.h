#pragma once

// Client Configuration Constants
namespace ClientConfig {
    // Progress reporting
    const int PROGRESS_UPDATE_INTERVAL = 100;  // Show progress every N attempts

    // Password generation
    const int MIN_PASSWORD_LENGTH = 1;
    const int MAX_PASSWORD_LENGTH = 20;
    
    // Pipe communication
    const int PIPE_BUFFER_SIZE = 512;
    
    // Timeout settings
    const unsigned long long PASSWORD_CRACKING_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutes
}
