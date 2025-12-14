#pragma once

namespace ServerConfig {
    // Налаштування каналів
    const int MAX_CONCURRENT_PIPES = 16;  // Максимальна кількість потоків
    const int PIPE_BUFFER_SIZE = 512;     // Розмір буфера
    
    // Налаштування захисту
    const unsigned long THROTTLE_DELAY_MS = 1000; // Затримка 1 секунда (1000 мс)
    const int MAX_ATTEMPTS_BEFORE_DELAY = 3;      // Поріг спроб перед увімкненням затримки
    
    // Налаштування потоків
    const unsigned long SHUTDOWN_TIMEOUT_MS = 5000; // Тайм-аут завершення
}