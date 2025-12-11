#pragma once

// Client Configuration Constants
namespace ClientConfig {
    // Progress reporting
    const int PROGRESS_UPDATE_INTERVAL = 100;  // Show progress every N attempts
    
    // Threading
    const int MIN_THREADS = 1;
    const int MAX_THREADS = 64;
    const int DEFAULT_THREADS = 4;
    
    // Password generation
    const int MIN_PASSWORD_LENGTH = 1;
    const int MAX_PASSWORD_LENGTH = 20;
    
    // Pipe communication
    const int PIPE_BUFFER_SIZE = 512;
}
