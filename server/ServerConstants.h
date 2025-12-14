#pragma once

// Server Configuration Constants
namespace ServerConfig {  // Number of simultaneous client connections (increased for multithreaded client)
    const int PIPE_BUFFER_SIZE = 512;     // Buffer size for pipe communication
    
    // Anti-brute-force protection
    const unsigned long BLOCK_DURATION_MS = 3000;   // Time to block account after failed attempt (3 seconds)
    const unsigned long RETRY_DELAY_MS = 1000;      // Delay before retrying blocked request (1 second)
    
    // Thread management
    const unsigned long SHUTDOWN_TIMEOUT_MS = 5000; // Maximum time to wait for thread shutdown (5 seconds)
}
