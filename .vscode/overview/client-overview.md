# Клиентское приложение - Детальный технический обзор

## Оглавление
1. [Общая архитектура](#1-общая-архитектура)
2. [Точка входа и главное меню](#2-точка-входа-и-главное-меню)
3. [Режимы работы клиента](#3-режимы-работы-клиента)
4. [Взаимодействие с сервером через Named Pipes](#4-взаимодействие-с-сервером-через-named-pipes)
5. [Многопоточная атака перебором](#5-многопоточная-атака-перебором)
6. [Атака по словарю с правилами](#6-атака-по-словарю-с-правилами)
7. [Вспомогательные классы](#7-вспомогательные-классы)
8. [Соответствие требованиям](#8-соответствие-требованиям)

---

## 1. Общая архитектура

### 1.1 Структура проекта

```
client/
├── main.cpp                        # Точка входа, главное меню
├── ClientConstants.h               # Конфигурационные константы
│
├── PipeClient.h/cpp               # Работа с Named Pipes (синхронный режим)
│
├── PasswordGenerator.h             # Абстрактный базовый класс (полиморфизм ООП)
├── BruteForce.h/cpp               # Генератор полного перебора
├── FirstLetterPartitionGenerator.h/cpp  # Партиционированный генератор для многопоточности
├── ParallelBruteForce.h/cpp       # Координация многопоточной атаки
│
├── RuleAttack.h/cpp               # Атака по словарю с правилами
├── LinkedList.h                    # Шаблонный однонаправленный список
│
├── Utils.h/cpp                    # Таймеры, статистика, консольный вывод
│
├── dictionary-config.txt          # Конфигурация правил и словарей
└── vocabularies/                  # Словари для атаки на основе правил
    ├── vocab_common_passwords.txt
    ├── vocab_keyboard_patterns.txt
    ├── vocab_ukrainian_russian_names.txt
    └── ... (12 файлов словарей)
```

### 1.2 Диаграмма классов

```
                    ┌─────────────────────┐
                    │  PasswordGenerator  │ (abstract)
                    │─────────────────────│
                    │ + HasNext(): bool   │
                    │ + Next(): string    │
                    │ + Reset(): void     │
                    │ + GetProgressPercent()│
                    │ + GetTotalCount()   │
                    └──────────┬──────────┘
                               │
           ┌───────────────────┼───────────────────┐
           ▼                   ▼                   ▼
┌─────────────────────┐ ┌──────────────────┐ ┌─────────────────┐
│ BruteForceGenerator │ │FirstLetterPartition│ │ RuleBasedAttack│
│─────────────────────│ │   Generator      │ │─────────────────│
│- alphabet: string   │ │──────────────────│ │- m_ruleSets     │
│- maxLength: int     │ │- myFirstLetters  │ │- dictionary     │
│- currentPassword    │ │- alphabet        │ │- variants       │
│- attemptCount       │ │- maxLength       │ │- variantSet     │
│- totalCombinations  │ │- finished        │ │- currentIndex   │
│─────────────────────│ │──────────────────│ │─────────────────│
│+ IncrementPassword()│ │+ Increment()     │ │+ ApplyAllRules()│
│+ CalculateTotal()   │ │+ CalculateTotal()│ │+ GenerateVariants()│
└─────────────────────┘ └──────────────────┘ └─────────────────┘

┌─────────────────────┐     ┌──────────────────┐
│     PipeClient      │     │   AttackContext  │
│─────────────────────│     │──────────────────│
│- hPipe: HANDLE      │     │- passwordFound   │ (atomic)
│- pipeName: string   │     │- timeoutReached  │ (atomic)
│- lastComputerName   │     │- totalAttempts   │ (atomic)
│─────────────────────│     │- startTime       │
│+ Connect()          │     │- foundPassword   │
│+ TryPassword()      │     │- targetLogin     │
│+ Disconnect()       │     │──────────────────│
│+ SendData()         │     │+ MarkFound()     │
│+ ReceiveResponse()  │     │+ ShouldStop()    │
│+ Reconnect()        │     │+ IsTimedOut()    │
└─────────────────────┘     └──────────────────┘

┌──────────────────────────────────────────────────────┐
│                  LinkedList<T>                       │
│──────────────────────────────────────────────────────│
│- head, tail: Node*                                   │
│- count: size_t                                       │
│──────────────────────────────────────────────────────│
│+ push_back(), push_front(), pop_front()              │
│+ operator[], size(), empty(), clear()                │
│+ begin(), end() - для range-based for               │
│+ Iterator, ConstIterator - вложенные классы         │
└──────────────────────────────────────────────────────┘
```

---

## 2. Точка входа и главное меню

### 2.1 Файл `main.cpp`

**Процесс запуска:**
1. Программа входит в бесконечный цикл `while(true)`
2. Отображается меню с 4 опциями
3. Пользователь вводит выбор (0-3)
4. Вызывается соответствующий режим работы
5. После выполнения - пауза и возврат к меню

```cpp
int main() {
    int choice = 0;
    while (true) {
        ShowMainMenu();                    // Отрисовка меню
        std::cin >> choice;                // Получение выбора
        std::cin.ignore(10000, '\n');      // Очистка буфера ввода
        
        switch (choice) {
            case 1: ModeConnectionTest();  break;   // Проверка связи
            case 2: ModeBruteForce();      break;   // Полный перебор
            case 3: ModeRuleBasedAttack(); break;   // Атака по словарю
            case 0: return 0;                       // Выход
        }
        Pause();  // Ожидание нажатия Enter
    }
}
```

### 2.2 Меню пользователя

```
======================================================================
 PASSWORD CRACKING CLIENT - Educational Version
======================================================================
 1. Connection Test       - Verify server connectivity
 2. Brute Force Attack    - Try all password combinations
 3. Dictionary Attack     - Rule-based dictionary attack
 0. Exit
======================================================================
```

---

## 3. Режимы работы клиента

### 3.1 Режим 1: Проверка связи (`ModeConnectionTest`)

**Назначение:** Верификация работоспособности соединения с сервером и проверка известных учётных данных.

**Алгоритм работы:**
```
1. Создание объекта PipeClient
2. Подключение к серверу: \\.\pipe\AuthPipe
3. Если неудача → вывод ошибки → выход
4. Запрос логина и пароля у пользователя
5. Отправка данных на сервер через TryPassword()
6. Получение ответа: 1 = успех, 0 = неудача
7. Вывод результата
8. Отключение от сервера
```

**Технические детали:**
- Используется для тестирования с известным паролем (тестовый логин/пароль из requirements)
- Один запрос → один ответ → отключение
- Позволяет проверить сетевую доступность сервера

### 3.2 Режим 2: Полный перебор (`ModeBruteForce`)

**Назначение:** Перебор всех возможных комбинаций паролей с использованием многопоточности.

**Полный алгоритм работы:**

```
1. ВЫБОР АЛФАВИТА
   ├── Опция 1: 27 символов (a-z + ')
   ├── Опция 2: 63 символа (A-Z, a-z, 0-9, ')
   └── Опция 3: 128 символов (латиница + кириллица + цифры + ')

2. ВВОД МАКСИМАЛЬНОЙ ДЛИНЫ ПАРОЛЯ (1-20)

3. ВВОД ЦЕЛЕВОГО ЛОГИНА

4. ОПРЕДЕЛЕНИЕ КОЛИЧЕСТВА ПОТОКОВ
   ├── Автодетект: std::thread::hardware_concurrency()
   └── Пользователь может выбрать от 1 до N аппаратных потоков

5. СОЗДАНИЕ ОБЩЕГО КОНТЕКСТА АТАКИ (AttackContext)
   ├── passwordFound (atomic<bool>) = false
   ├── timeoutReached (atomic<bool>) = false
   ├── totalAttempts (atomic<ull>) = 0
   ├── startTime = now()
   └── targetLogin = введённый логин

6. ПАРТИЦИОНИРОВАНИЕ АЛФАВИТА
   ├── Функция: PartitionAlphabet(alphabet, threadCount)
   ├── Алфавит делится на N частей
   └── Каждый поток получает свои "первые буквы"

7. ЗАПУСК РАБОЧИХ ПОТОКОВ
   for (i = 0; i < threadCount; i++) {
       workers.emplace_back(BruteForceWorker, i, alphabet, 
                           maxLength, partitions[i], &context);
   }

8. МОНИТОРИНГ ПРОГРЕССА (главный поток)
   while (!context.ShouldStop()) {
       sleep(500ms);
       Вывод: attempts | passwords/sec | threads | timeout
   }

9. ОЖИДАНИЕ ЗАВЕРШЕНИЯ ВСЕХ ПОТОКОВ
   for (auto& t : workers) { t.join(); }

10. ВЫВОД РЕЗУЛЬТАТОВ
    ├── При успехе: логин, пароль, потоки, попытки, время, скорость
    ├── При таймауте: уведомление о превышении 5 минут
    └── При неудаче: количество попыток и время
```

**Пример вывода во время работы:**
```
Attempts: 1234567 | 45678.9 pwd/sec | Threads: 8 | Timeout: 245s
```

### 3.3 Режим 3: Атака по словарю (`ModeRuleBasedAttack`)

**Назначение:** Генерация вариантов паролей на основе словарей и правил трансформации.

**Алгоритм работы:**

```
1. ВЫБОР РЕЖИМА ЗАГРУЗКИ
   ├── Опция 1: Конфигурационный файл (маппинг правило → словарь)
   └── Опция 2: Один словарь (все правила применяются)

2. ЗАГРУЗКА ДАННЫХ
   Опция 1 (конфиг файл):
   ├── Открытие диалога GetOpenFileNameA()
   ├── Парсинг файла: RULE_TYPE vocabulary_path
   └── Загрузка каждого словаря в LinkedList

   Опция 2 (один словарь):
   ├── Открытие диалога GetOpenFileNameA()
   └── Загрузка слов в LinkedList<string>

3. ГЕНЕРАЦИЯ ВАРИАНТОВ
   ├── Вызов GenerateVariants()
   ├── Для каждого слова применяются правила
   ├── Результаты сохраняются в LinkedList<string> variants
   └── Дедупликация через unordered_set<string> variantSet

4. ВВОД ЦЕЛЕВОГО ЛОГИНА

5. ПОДКЛЮЧЕНИЕ К СЕРВЕРУ

6. ПОСЛЕДОВАТЕЛЬНАЯ АТАКА
   while ((password = attack.Next()) != "" && !found && !timedOut) {
       attemptCount++;
       
       // Проверка таймаута (5 минут)
       if (elapsed >= PASSWORD_CRACKING_TIMEOUT_MS) {
           timedOut = true;
           break;
       }
       
       // Попытка авторизации
       if (client.TryPassword(targetLogin, password)) {
           found = true;
           // Вывод результата
       }
       
       // Периодический вывод прогресса
       if (attemptCount % 100 == 0) {
           Console::ClearLine();
           // Вывод: attempt N: password | rate | timeout
       }
   }

7. ВЫВОД РЕЗУЛЬТАТОВ
```

---

## 4. Взаимодействие с сервером через Named Pipes

### 4.1 Класс `PipeClient`

**Файлы:** `PipeClient.h`, `PipeClient.cpp`

**Конфигурация:**
```cpp
// Имя канала по умолчанию
std::string pipeName = "\\\\.\\pipe\\AuthPipe";

// Размер буфера
static const int BUFFER_SIZE = 512;

// Режим работы: СИНХРОННЫЙ (по требованиям)
```

### 4.2 Метод `Connect()`

**Процесс подключения:**
```cpp
bool PipeClient::Connect(const std::string& computerName) {
    // 1. Формирование полного имени канала
    //    Локально: \\.\pipe\AuthPipe
    //    По сети:  \\COMPUTER_NAME\pipe\AuthPipe
    
    // 2. Первая попытка подключения
    hPipe = CreateFileA(fullPipeName.c_str(), 
                        GENERIC_READ | GENERIC_WRITE,  // Двунаправленный доступ
                        0,                              // Без совместного доступа
                        NULL,                           // Дефолтные атрибуты
                        OPEN_EXISTING,                  // Канал должен существовать
                        0,                              // Синхронный режим
                        NULL);
    
    // 3. Обработка занятости канала (ERROR_PIPE_BUSY)
    if (GetLastError() == ERROR_PIPE_BUSY) {
        WaitNamedPipeA(fullPipeName.c_str(), 5000);  // Ожидание 5 секунд
        // Повторная попытка подключения
    }
    
    return (hPipe != INVALID_HANDLE_VALUE);
}
```

### 4.3 Метод `TryPassword()`

**ВАЖНО:** Каждая попытка = отдельное подключение (по архитектуре сервера).

```cpp
bool PipeClient::TryPassword(const std::string& login, const std::string& password) {
    // 1. Подключение к серверу
    if (!Connect(lastComputerName)) {
        return false;
    }
    
    // 2. Формирование сообщения: "login password"
    std::string message = login + " " + password;
    
    // 3. Отправка данных
    if (!SendData(message)) {
        Disconnect();
        return false;
    }
    
    // 4. Получение ответа (DWORD: 0 или 1)
    DWORD response = 0;
    if (!ReceiveResponse(response)) {
        Disconnect();
        return false;
    }
    
    // 5. Отключение после каждой попытки
    Disconnect();
    
    // 6. Возврат результата
    return (response == 1);  // 1 = пароль верный
}
```

### 4.4 Обработка ошибок

**Типы обрабатываемых ошибок:**
```cpp
ERROR_PIPE_BUSY         // Канал занят → ожидание WaitNamedPipe
ERROR_BROKEN_PIPE       // Сервер отключился → попытка реконнекта
ERROR_PIPE_NOT_CONNECTED// Канал не подключен → попытка реконнекта
ERROR_INVALID_HANDLE    // Невалидный хэндл → логирование
```

**Стратегия переподключения:**
```cpp
bool PipeClient::Reconnect() {
    Disconnect();                    // Закрытие текущего соединения
    return Connect(lastComputerName);// Повторное подключение
}
```

### 4.5 Протокол обмена данными

```
┌──────────┐                              ┌──────────┐
│  Client  │                              │  Server  │
└────┬─────┘                              └────┬─────┘
     │                                         │
     │  CreateFile() / Connect                 │
     │────────────────────────────────────────>│
     │                                         │
     │  WriteFile("login password")            │
     │────────────────────────────────────────>│
     │                                         │
     │             [Проверка]                  │
     │                                         │
     │  ReadFile() ← response (DWORD: 0/1)     │
     │<────────────────────────────────────────│
     │                                         │
     │  CloseHandle() / Disconnect             │
     │────────────────────────────────────────>│
     │                                         │
```

---

## 5. Многопоточная атака перебором

### 5.1 Архитектура распараллеливания

**Стратегия:** Партиционирование по первой букве пароля.

```
Алфавит: "abcdefghijklmnopqrstuvwxyz'" (27 символов)
Потоки: 4

Распределение:
┌────────────────────────────────────────────────────┐
│ Поток 0: a, b, c, d, e, f, g     (7 букв)         │
│ Поток 1: h, i, j, k, l, m, n     (7 букв)         │
│ Поток 2: o, p, q, r, s, t        (6 букв)         │
│ Поток 3: u, v, w, x, y, z, '     (7 букв)         │
└────────────────────────────────────────────────────┘

Каждый поток перебирает ВСЕ пароли, начинающиеся с его букв:
Поток 0: a, aa, ab, ..., az, a', aaa, aab, ..., b, ba, bb, ...
```

### 5.2 Функция партиционирования

```cpp
std::vector<std::vector<char>> PartitionAlphabet(const std::string& alphabet, 
                                                  int threadCount) {
    std::vector<std::vector<char>> partitions;
    
    int charsPerThread = alphabet.length() / threadCount;
    int remainder = alphabet.length() % threadCount;
    
    int startIdx = 0;
    for (int i = 0; i < threadCount; i++) {
        // Первые 'remainder' потоков получают +1 символ для балансировки
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
```

### 5.3 Рабочий поток (`BruteForceWorker`)

```cpp
void BruteForceWorker(int threadId,
                      const std::string& alphabet,
                      int maxLength,
                      const std::vector<char>& myFirstLetters,
                      AttackContext* context)
{
    PipeClient client;  // Каждый поток имеет свой экземпляр клиента
    
    // Создание генератора для назначенных первых букв
    FirstLetterPartitionGenerator generator(alphabet, maxLength, myFirstLetters);
    
    // Основной цикл перебора
    while (generator.HasNext() && !context->ShouldStop()) {
        std::string password = generator.Next();
        
        context->IncrementAttempts();  // Atomic increment
        
        if (client.TryPassword(context->targetLogin, password)) {
            context->MarkFound(password);  // Thread-safe установка результата
            break;
        }
    }
}
```

### 5.4 Структура `AttackContext` (потокобезопасная)

```cpp
struct AttackContext {
    // Атомарные флаги состояния
    std::atomic<bool> passwordFound;        // Пароль найден?
    std::atomic<bool> timeoutReached;       // Таймаут достигнут?
    std::atomic<unsigned long long> totalAttempts;  // Общее число попыток
    
    // Время запуска атаки
    std::chrono::high_resolution_clock::time_point startTime;
    
    // Защита результата мьютексом
    std::mutex resultMutex;
    std::string foundPassword;
    
    // Целевой логин (константа)
    const std::string targetLogin;
    
    // Конструктор
    explicit AttackContext(const std::string& login)
        : passwordFound(false), timeoutReached(false), 
          totalAttempts(0), targetLogin(login),
          startTime(std::chrono::high_resolution_clock::now()) {}
    
    // Потокобезопасная установка найденного пароля
    void MarkFound(const std::string& password) {
        std::lock_guard<std::mutex> lock(resultMutex);
        if (foundPassword.empty()) {  // Первый нашедший "побеждает"
            foundPassword = password;
            passwordFound.store(true);
        }
    }
    
    // Проверка условия остановки
    bool ShouldStop() {
        CheckAndSetTimeout();
        return IsFound() || IsTimedOut();
    }
    
    // Проверка таймаута (5 минут)
    bool IsTimedOut() const {
        if (timeoutReached.load()) return true;
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTime).count();
        return elapsed >= ClientConfig::PASSWORD_CRACKING_TIMEOUT_MS;
    }
};
```

### 5.5 Генератор `FirstLetterPartitionGenerator`

**Инициализация:**
```cpp
FirstLetterPartitionGenerator(fullAlphabet, maxLength, firstLetters)
│
├── alphabet = fullAlphabet       // Полный алфавит для позиций 1+
├── myFirstLetters = firstLetters // Мои назначенные первые буквы
├── maxLength = maxLen
├── currentFirstLetterIdx = 0     // Индекс текущей первой буквы
├── currentPassword = firstLetters[0]  // Начинаем с первой буквы
├── attemptCount = 0
└── finished = false
```

**Алгоритм инкремента (Increment):**
```
Пример для alphabet="ab", myFirstLetters=['a'], maxLength=3:

Последовательность паролей:
a → aa → ab → aaa → aab → aba → abb → aaa (переход к длине 3)

1. Если длина = 1:
   - Если maxLength > 1: добавляем alphabet[0] → "aa"
   - Иначе: переходим к следующей первой букве
   
2. Иначе (длина > 1):
   - Пытаемся инкрементировать с правой позиции
   - pos = length - 1
   - while (pos > 0):  // Не трогаем позицию 0
       - Если можно инкрементировать символ → делаем и выходим
       - Иначе → сбрасываем в alphabet[0], pos--
   - Если все позиции переполнились:
       - Если можно увеличить длину → добавляем alphabet[0]
       - Иначе → переходим к следующей первой букве

3. Если первые буквы закончились → finished = true
```

### 5.6 Подсчёт комбинаций

**Формула для одной первой буквы:**
```
N = 1 + |A| + |A|² + ... + |A|^(maxLen-1)
    ↑    ↑     ↑
    len=1 len=2 len=3

Где |A| - размер алфавита
```

**Общее количество для партиции:**
```
N_total = |myFirstLetters| × (1 + |A| + |A|² + ... + |A|^(maxLen-1))
```

---

## 6. Атака по словарю с правилами

### 6.1 Класс `RuleBasedAttack`

**Файлы:** `RuleAttack.h`, `RuleAttack.cpp`

**Основные компоненты:**
```cpp
class RuleBasedAttack : public PasswordGenerator {
private:
    // Режим 1: Конфигурационный файл
    LinkedList<RuleSet> m_ruleSets;    // Список пар (правило, словарь)
    
    // Режим 2: Один словарь
    LinkedList<std::string> dictionary;
    
    // Сгенерированные варианты
    LinkedList<std::string> variants;           // Для последовательного доступа
    std::unordered_set<std::string> variantSet; // Для O(1) дедупликации
    size_t currentIndex;                        // Текущая позиция итерации
    
    // Маппинги для транслитерации
    static const std::map<char, std::string> latinToCyrillic;
    static const std::map<char, char> cyrillicToLatin;
};
```

### 6.2 Типы правил (`RuleType`)

```cpp
enum class RuleType {
    INVALID = 0,
    CASE_VARIATIONS,       // Вариации регистра: hello, HELLO, Hello
    DIGIT_SUFFIXES,        // Цифровые суффиксы: pass1, pass12, pass2024
    CYRILLIC_LAYOUT,       // Кириллическая раскладка
    CHARACTER_TRANSPOSE,   // Перестановка символов: hello → hlelo
    STRING_REVERSAL,       // Реверс строки: hello → olleh
    LATIN_TO_CYRILLIC,     // Латиница → кириллица
    SPECIAL_CHARS,         // Спецсимволы: pass!, pass@, pass#
    YEAR_SUFFIXES,         // Годы: pass1995, pass2024
    NAME_CITY_COMBO,       // Имя+город: ivan+kyiv
    MULTIPLE_EXCLAMATION,  // Восклицания: pass!, pass!!, pass!!!
    DATE_SUFFIXES,         // Даты: pass19950315
    LONG_SEQUENCES,        // Длинные последовательности: pass123456789
    COMPLEX_SPECIAL_COMBOS // Сложные комбо: Xj8!Kv4@
};
```

### 6.3 Формат конфигурационного файла

**Файл:** `dictionary-config.txt`

```
# Комментарии начинаются с #
# Формат: RULE_TYPE vocabulary_file_path

CASE_VARIATIONS vocabularies/vocab_common_passwords.txt
DIGIT_SUFFIXES vocabularies/vocab_keyboard_patterns.txt
CYRILLIC_LAYOUT vocabularies/vocab_russian_cyrillic.txt
MULTIPLE_EXCLAMATION vocabularies/vocab_for_multiple_exclamation.txt
DATE_SUFFIXES vocabularies/vocab_for_dates.txt
...
```

### 6.4 Алгоритм загрузки конфигурации

```cpp
bool RuleBasedAttack::LoadConfigFile(const std::string& configPath) {
    // 1. Открытие файла конфигурации
    std::ifstream configFile(configPath);
    
    // 2. Очистка предыдущего состояния
    m_ruleSets.clear();
    variants.clear();
    variantSet.clear();
    
    // 3. Парсинг строк
    while (std::getline(configFile, line)) {
        // Пропуск пустых строк и комментариев
        if (trimmed.empty() || trimmed[0] == '#') continue;
        
        // Разбор: "RULE_TYPE path"
        size_t spacePos = trimmed.find(' ');
        std::string ruleTypeString = trimmed.substr(0, spacePos);
        std::string vocabularyPath = trimmed.substr(spacePos + 1);
        
        // Парсинг типа правила
        RuleType ruleType = ParseRuleType(ruleTypeString);
        if (ruleType == RuleType::INVALID) continue;
        
        // Создание и загрузка набора правил
        RuleSet ruleSet(ruleType);
        if (!LoadVocabularyFile(vocabularyPath, ruleSet.vocabulary)) continue;
        
        m_ruleSets.push_back(ruleSet);
    }
    
    return m_ruleSets.size() > 0;
}
```

### 6.5 Генерация вариантов

**Режим 1: Конфигурационный файл**
```cpp
void RuleBasedAttack::GenerateVariants() {
    variants.clear();
    variantSet.clear();
    
    // Для каждого набора правил
    for (const auto& ruleSet : m_ruleSets) {
        // Для каждого слова в словаре набора
        for (const auto& word : ruleSet.vocabulary) {
            // Применяем только указанное правило
            ApplySpecificRule(ruleSet.type, word);
        }
    }
}
```

**Режим 2: Один словарь (все правила)**
```cpp
// Если словарь загружен напрямую
for (const auto& basePassword : dictionary) {
    ApplyAllRules(basePassword);
}
```

### 6.6 Реализация правил

#### CASE_VARIATIONS
```cpp
void RuleBasedAttack::GenerateCaseVariations(const std::string& base) {
    AddVariant(base);                    // original
    AddVariant(ToLower(base));           // hello
    AddVariant(ToUpper(base));           // HELLO
    AddVariant(Capitalize(base));        // Hello (первая заглавная)
}
```

#### DIGIT_SUFFIXES
```cpp
void RuleBasedAttack::GenerateDigitSuffixes(const std::string& base) {
    AddVariant(base);
    AddVariant(base + "1");
    AddVariant(base + "12");
    AddVariant(base + "123");
    AddVariant(base + "1234");
    AddVariant(base + "2023");
    AddVariant(base + "2024");
    AddVariant(base + "2025");
    AddVariant(base + "0");
    AddVariant(base + "00");
    AddVariant(base + "123456");
    AddVariant(base + "!");
}
```

#### STRING_REVERSAL
```cpp
std::string RuleBasedAttack::ReversePassword(const std::string& password) {
    return std::string(password.rbegin(), password.rend());
    // "hello" → "olleh"
}
```

#### CHARACTER_TRANSPOSE
```cpp
std::vector<std::string> RuleBasedAttack::TransposeCharacters(const std::string& password) {
    std::vector<std::string> transposed;
    
    // Генерируем все варианты с перестановкой соседних символов
    for (size_t i = 0; i < password.length() - 1; i++) {
        std::string variant = password;
        std::swap(variant[i], variant[i + 1]);
        transposed.push_back(variant);
    }
    // "hello" → ["ehllo", "hlelo", "hello", "helol"]
    
    return transposed;
}
```

#### LATIN_TO_CYRILLIC
```cpp
std::string RuleBasedAttack::LatinToCyrillic(const std::string& latin) {
    // Маппинг фонетический:
    // a→а, b→б, c→ц, d→д, e→е, ...
    // "hello" → "хелло" (примерно)
}
```

#### CYRILLIC_TO_LATIN (раскладка клавиатуры)
```cpp
std::string RuleBasedAttack::CyrillicToLatin(const std::string& cyrillic) {
    // Маппинг по расположению клавиш:
    // а→f, б→,, в→d, г→u, д→l, ...
    // "привет" → "ghbdtn"
}
```

#### DATE_SUFFIXES
```cpp
void RuleBasedAttack::GenerateDateSuffixes(const std::string& base) {
    AddVariant(base);
    
    // Форматы: YYYYMMDD
    for (int year = 1985; year <= 2005; year++) {
        for (int month = 1; month <= 12; month++) {
            for (int day : {1, 5, 10, 15, 20, 25, 28}) {
                char dateStr[9];
                sprintf(dateStr, "%04d%02d%02d", year, month, day);
                AddVariant(base + std::string(dateStr));
            }
        }
    }
    // "ivan" → "ivan19951210", "ivan20000315", ...
}
```

#### COMPLEX_SPECIAL_COMBOS
```cpp
void RuleBasedAttack::GenerateComplexSpecialCombos(const std::string& base) {
    // Паттерны: Letter+Digit+Special (Xj8!Kv4@)
    static const std::vector<std::string> patterns = {
        "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9!"
    };
    
    // Двухчастные комбинации
    for (size_t i = 0; i < patterns.size(); i++) {
        for (size_t j = 0; j < patterns.size(); j++) {
            if (i != j) {
                AddVariant(base + patterns[i] + patterns[j]);
            }
        }
    }
    // "Xj" → "Xj1!2@", "Xj8!4@", ...
}
```

### 6.7 Дедупликация вариантов

```cpp
void RuleBasedAttack::AddVariant(const std::string& variant) {
    // O(1) проверка на уникальность через unordered_set
    if (variantSet.find(variant) == variantSet.end()) {
        variantSet.insert(variant);      // Для быстрой проверки
        variants.push_back(variant);     // Для последовательного доступа
    }
}
```

### 6.8 Структура словарей

**Файлы в директории `vocabularies/`:**

| Файл | Содержимое |
|------|------------|
| `vocab_common_passwords.txt` | Общеизвестные пароли: password, qwerty, 123456 |
| `vocab_keyboard_patterns.txt` | Клавиатурные паттерны: qwerty, asdf, zxcv |
| `vocab_ukrainian_russian_names.txt` | Имена: Anna, Ivan, Olga, Maks |
| `vocab_russian_cyrillic.txt` | Кириллические слова: пароль, привет |
| `vocab_personal_info.txt` | Персональные данные: имена, даты |
| `vocab_cities.txt` | Города: Kyiv, Lviv, Moscow |
| `vocab_for_dates.txt` | Базы для дат |
| `vocab_for_multiple_exclamation.txt` | Базы для восклицаний |
| `vocab_two_letter_combos.txt` | Двухбуквенные комбо: Xj, Pb, Df |
| `vocab_random_combinations.txt` | Случайные комбинации |
| `vocab_short_words.txt` | Короткие слова |
| `vocab_common_sequences.txt` | Общие последовательности |

---

## 7. Вспомогательные классы

### 7.1 Шаблонный класс `LinkedList<T>`

**Файл:** `LinkedList.h`

**Реализация однонаправленного связанного списка (по требованиям):**

```cpp
template<typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& value) : data(value), next(nullptr) {}
    };
    
    Node* head;
    Node* tail;
    size_t count;

public:
    // Конструкторы и деструкторы
    LinkedList();                           // По умолчанию
    ~LinkedList();                          // Деструктор - освобождение памяти
    LinkedList(const LinkedList& other);    // Копирования
    LinkedList(LinkedList&& other) noexcept;// Перемещения
    
    // Операции модификации
    void push_back(const T& value);   // O(1) - добавление в конец
    void push_front(const T& value);  // O(1) - добавление в начало
    void pop_front();                 // O(1) - удаление из начала
    void clear();                     // O(n) - очистка списка
    
    // Доступ к элементам
    T& front();                       // O(1)
    T& back();                        // O(1)
    T& operator[](size_t index);      // O(n) - доступ по индексу
    
    // Информация
    size_t size() const;
    bool empty() const;
    
    // Итераторы для range-based for
    class Iterator;
    class ConstIterator;
    Iterator begin();
    Iterator end();
    ConstIterator cbegin() const;
    ConstIterator cend() const;
};
```

**Вложенный класс `Iterator`:**
```cpp
class Iterator {
private:
    Node* current;
public:
    Iterator(Node* node) : current(node) {}
    
    T& operator*() { return current->data; }
    Iterator& operator++() {
        if (current) current = current->next;
        return *this;
    }
    bool operator!=(const Iterator& other) const {
        return current != other.current;
    }
};
```

### 7.2 Класс `Timer`

**Файл:** `Utils.h`, `Utils.cpp`

```cpp
class Timer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    bool isRunning;

public:
    void Start() {
        startTime = std::chrono::high_resolution_clock::now();
        isRunning = true;
    }
    
    unsigned long long Stop() {
        isRunning = false;
        auto endTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
    }
    
    unsigned long long GetElapsed() const {
        auto currentTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();
    }
    
    std::string GetElapsedFormatted() const {
        // Возвращает: "1h 23m 45s" или "5m 30s" или "12.5s"
    }
};
```

### 7.3 Класс `AttackStats`

```cpp
class AttackStats {
private:
    unsigned long long totalAttempts;
    unsigned long long startTime;   // GetTickCount64()
    unsigned long long endTime;

public:
    void Start() { startTime = GetTickCount64(); }
    void Stop() { endTime = GetTickCount64(); }
    void RecordAttempt() { totalAttempts++; }
    void AddAttempts(unsigned long long count) { totalAttempts += count; }
    
    double GetAttemptsPerSecond() const {
        unsigned long long elapsedMs = GetElapsedMs();
        if (elapsedMs == 0) return 0.0;
        return (totalAttempts * 1000.0) / elapsedMs;
    }
    
    std::string GetStats() const {
        // "Attempts: 123456 | Rate: 4567.8 pwd/sec | Time: 5m 30s"
    }
};
```

### 7.4 Пространство имён `Console`

**Файл:** `Utils.h`, `Utils.cpp`

**Цветной вывод через WinAPI:**
```cpp
namespace Console {
    enum Color {
        BLACK = 0, DARK_BLUE = 1, ..., WHITE = 15
    };
    
    void SetColor(Color foreground, Color background = BLACK) {
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(handle, (background << 4) | foreground);
    }
    
    void PrintSuccess(const std::string& message) {
        // Зелёный: [+] message
        PrintColored(LIGHT_GREEN, "[+] " + message);
    }
    
    void PrintError(const std::string& message) {
        // Красный: [!] message
        PrintColored(LIGHT_RED, "[!] " + message);
    }
    
    void PrintInfo(const std::string& message) {
        // Голубой: [*] message
        PrintColored(LIGHT_CYAN, "[*] " + message);
    }
    
    void PrintWarning(const std::string& message) {
        // Жёлтый: [?] message
        PrintColored(LIGHT_YELLOW, "[?] " + message);
    }
    
    void ClearLine() {
        // Очистка текущей строки консоли
        std::cout << "\r" << std::string(120, ' ') << "\r";
    }
    
    void PrintHeader(const std::string& title) {
        // ==============================
        //   TITLE
        // ==============================
    }
}
```

### 7.5 Константы конфигурации

**Файл:** `ClientConstants.h`

```cpp
namespace ClientConfig {
    // Интервал обновления прогресса
    const int PROGRESS_UPDATE_INTERVAL = 100;
    
    // Ограничения длины пароля
    const int MIN_PASSWORD_LENGTH = 1;
    const int MAX_PASSWORD_LENGTH = 20;
    
    // Размер буфера pipe
    const int PIPE_BUFFER_SIZE = 512;
    
    // Таймаут атаки: 5 минут
    const unsigned long long PASSWORD_CRACKING_TIMEOUT_MS = 5 * 60 * 1000;
}
```

---

## 8. Соответствие требованиям

### 8.1 Требования к клиентскому приложению

| Требование | Статус | Реализация |
|------------|--------|------------|
| **Режим проверки связи** | ✅ | `ModeConnectionTest()` - ввод логина/пароля, отправка на сервер, получение результата |
| **Режим взлома перебором** | ✅ | `ModeBruteForce()` - выбор алфавита, длины, многопоточный перебор |
| **Алфавит 27 символов** | ✅ | `"abcdefghijklmnopqrstuvwxyz'"` |
| **Алфавит 63 символа** | ✅ | `"A-Z, a-z, 0-9, '"` |
| **Алфавит 128 символов** | ✅ | Латиница + кириллица + цифры + апостроф |
| **Максимальная длина пароля** | ✅ | Ввод пользователем (1-20) |
| **Логин для взлома** | ✅ | Ввод пользователем |

### 8.2 Требования к атаке на основе правил (100%+)

| Требование | Статус | Реализация |
|------------|--------|------------|
| **Правила как методы класса** | ✅ | `RuleBasedAttack::GenerateCaseVariations()`, `GenerateDigitSuffixes()`, etc. |
| **a) Замена букв (раскладка)** | ✅ | `LatinToCyrillic()`, `CyrillicToLatin()` |
| **b) Изменение регистра** | ✅ | `GenerateCaseVariations()` |
| **c) Изменение порядка букв** | ✅ | `ReversePassword()`, `TransposeCharacters()` |
| **Ввод файла через GetOpenFileName** | ✅ | `LoadConfigFile()`, `LoadDictionaryFromFile()` |
| **Хранение в однонаправленных списках** | ✅ | `LinkedList<T>` - шаблонный класс |

### 8.3 Требования к многопоточности (140%+)

| Требование | Статус | Реализация |
|------------|--------|------------|
| **Многопоточность клиента** | ✅ | `std::thread`, `BruteForceWorker()` |
| **Потокобезопасная синхронизация** | ✅ | `std::atomic`, `std::mutex` в `AttackContext` |
| **Автодетект количества потоков** | ✅ | `std::thread::hardware_concurrency()` |
| **Партиционирование нагрузки** | ✅ | `PartitionAlphabet()`, `FirstLetterPartitionGenerator` |

### 8.4 Требования к Named Pipes

| Требование | Статус | Реализация |
|------------|--------|------------|
| **Синхронный режим клиента** | ✅ | `CreateFileA()` без `FILE_FLAG_OVERLAPPED` |
| **Имя канала согласовано** | ✅ | `\\.\pipe\AuthPipe` |
| **Обработка ERROR_PIPE_BUSY** | ✅ | `WaitNamedPipeA()` с таймаутом 5 сек |
| **Работа по сети** | ✅ | Поддержка `\\COMPUTERNAME\pipe\AuthPipe` |
| **Протокол: логин пароль** | ✅ | Формат сообщения: `"login password"` |
| **Ответ: DWORD 0/1** | ✅ | `ReceiveResponse(DWORD& response)` |

### 8.5 Технические требования

| Требование | Статус | Реализация |
|------------|--------|------------|
| **ООП обязательно** | ✅ | Классы: `PipeClient`, `BruteForceGenerator`, `RuleBasedAttack`, `LinkedList<T>`, `Timer`, `AttackStats` |
| **Наследование и полиморфизм** | ✅ | `PasswordGenerator` (абстрактный) → `BruteForceGenerator`, `FirstLetterPartitionGenerator`, `RuleBasedAttack` |
| **Без рекурсии при взломе** | ✅ | Итеративные алгоритмы: `IncrementPassword()`, `Increment()` |
| **Вывод с пояснениями** | ✅ | `Console::PrintInfo()`, `PrintSuccess()`, `PrintError()` |
| **Время поиска** | ✅ | `Timer::GetElapsedFormatted()`, вывод в результатах |

### 8.6 Структура файлов клиента (по требованиям)

| Требуемый файл | Реализация |
|----------------|------------|
| `PipeClient.h` | ✅ `PipeClient.h` - класс для работы с Named Pipe |
| `test.cpp` (main) | ✅ `main.cpp` - вспомогательные функции + main |
| `list.h` (шаблонный список) | ✅ `LinkedList.h` |

**Дополнительные файлы:**
- `BruteForce.h/cpp` - генератор полного перебора
- `ParallelBruteForce.h/cpp` - многопоточная атака
- `FirstLetterPartitionGenerator.h/cpp` - партиционированный генератор
- `RuleAttack.h/cpp` - атака по словарю
- `PasswordGenerator.h` - абстрактный базовый класс
- `Utils.h/cpp` - таймеры и консольный вывод
- `ClientConstants.h` - константы конфигурации

---

## Приложение А: Диаграмма процесса полного перебора

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ModeBruteForce()                            │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Выбор алфавита (1-3) │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Ввод макс. длины     │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Ввод логина          │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Выбор кол-ва потоков │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Создание AttackContext│
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  PartitionAlphabet()  │
                    └───────────┬───────────┘
                                │
           ┌────────────────────┼────────────────────┐
           ▼                    ▼                    ▼
    ┌─────────────┐      ┌─────────────┐      ┌─────────────┐
    │  Worker 0   │      │  Worker 1   │      │  Worker N   │
    │ letters:a-g │      │ letters:h-n │      │ letters:u-' │
    └──────┬──────┘      └──────┬──────┘      └──────┬──────┘
           │                    │                    │
           ▼                    ▼                    ▼
    ┌─────────────────────────────────────────────────────┐
    │              AttackContext (shared)                 │
    │  passwordFound, timeoutReached, totalAttempts       │
    └──────────────────────────┬──────────────────────────┘
                               │
                               ▼
                    ┌───────────────────────┐
                    │  Main thread:         │
                    │  - Мониторинг прогресса│
                    │  - Проверка ShouldStop│
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  join() всех потоков  │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Вывод результатов    │
                    └───────────────────────┘
```

---

## Приложение Б: Пример генерации вариантов

**Входное слово:** `"password"`

**Генерируемые варианты:**
```
1. GenerateCaseVariations():
   password, PASSWORD, Password

2. ReversePassword():
   drowssap → GenerateCaseVariations():
   drowssap, DROWSSAP, Drowssap

3. GenerateDigitSuffixes():
   password1, password12, password123, password1234,
   password2023, password2024, password2025,
   password0, password00, password123456, password!

4. LatinToCyrillic():
   пассворд → GenerateCaseVariations() + GenerateDigitSuffixes()

5. TransposeCharacters():
   apssword, psasword, passwrod, passowrd, passwor, ...
   + case variations для каждого

Итого: ~50-100 уникальных вариантов для одного слова
```

---

## Приложение В: Оценка производительности

### Теоретическое количество комбинаций

| Алфавит | Символов | Длина 3 | Длина 5 | Длина 7 |
|---------|----------|---------|---------|---------|
| Простой | 27 | 19,710 | 14.3M | 10.5B |
| Средний | 63 | 254,016 | 992M | 3.9T |
| Полный | 128 | 2.1M | 34.4B | 562T |

### Практическая скорость

**Факторы, влияющие на скорость:**
1. Сетевая задержка (Named Pipe)
2. Обработка на сервере
3. Количество потоков
4. Режим противодействия на сервере

**Типичная производительность:**
- Локально: ~1000-5000 паролей/сек
- По сети: ~100-500 паролей/сек
- С противодействием: значительно ниже

---

## Приложение Г: Технические аспекты реализации (подробно)

### 1. Многопоточность — как это работает

**Зачем нужна многопоточность?**

Перебор паролей — это задача, которая легко распараллеливается. Если у компьютера 8 ядер, то 8 потоков могут одновременно проверять разные пароли, ускоряя взлом примерно в 8 раз.

**Как реализовано:**

```cpp
// 1. Узнаём сколько ядер у процессора
unsigned int hwThreads = std::thread::hardware_concurrency();
// Например: 8 на Intel i7

// 2. Делим алфавит между потоками
// Если алфавит "abcdefgh" и 4 потока:
// Поток 0 → буквы "ab" (все пароли начинающиеся с a или b)
// Поток 1 → буквы "cd"
// Поток 2 → буквы "ef"
// Поток 3 → буквы "gh"

// 3. Создаём и запускаем потоки
std::vector<std::thread> workers;
for (int i = 0; i < threadCount; i++) {
    workers.emplace_back(BruteForceWorker, i, alphabet, maxLength, partitions[i], &context);
    // emplace_back создаёт поток и сразу запускает его
}

// 4. Главный поток ждёт завершения всех рабочих
for (auto& t : workers) {
    t.join();  // Блокируется пока поток не завершится
}
```

**Почему именно разделение по первой букве?**

Это простой и эффективный способ гарантировать, что потоки не пересекаются:
- Поток 0 никогда не проверит пароль "hello" (начинается на h)
- Поток 1 проверит все пароли на "h", включая "hello"
- Нет дублирования работы, нет конфликтов

---

### 2. Синхронизация потоков — как избежать хаоса

**Проблема:** 8 потоков работают одновременно. Что если два потока одновременно:
- Попытаются записать "найден пароль"?
- Увеличат счётчик попыток?
- Проверят флаг "пора останавливаться"?

**Решение 1: Атомарные переменные (для простых данных)**

```cpp
// Обычная переменная — ОПАСНО в многопоточности
bool found = false;
// Поток 1 читает: found = false
// Поток 2 читает: found = false  (одновременно!)
// Оба думают что пароль не найден — гонка данных

// Атомарная переменная — БЕЗОПАСНО
std::atomic<bool> passwordFound = false;
// Операции load() и store() выполняются целиком, без прерываний

// Проверка (читаем атомарно)
if (passwordFound.load()) { 
    // пароль найден другим потоком, останавливаемся
}

// Установка (пишем атомарно)
passwordFound.store(true);
```

**Для счётчика попыток:**
```cpp
std::atomic<unsigned long long> totalAttempts = 0;

// Каждый поток безопасно увеличивает
totalAttempts.fetch_add(1);  // Атомарный инкремент
// Даже если 8 потоков вызовут это одновременно — ни одна попытка не потеряется
```

**Решение 2: Мьютекс (для сложных данных)**

```cpp
// Строку нельзя сделать атомарной — она сложная
std::string foundPassword;
std::mutex resultMutex;

void MarkFound(const std::string& password) {
    // lock_guard автоматически захватывает мьютекс
    std::lock_guard<std::mutex> lock(resultMutex);
    
    // Только один поток может быть здесь одновременно
    if (foundPassword.empty()) {
        foundPassword = password;  // Безопасная запись
        passwordFound.store(true);
    }
    // При выходе из функции lock_guard автоматически освобождает мьютекс
}
```

**Почему `lock_guard`?**
- Автоматически захватывает мьютекс при создании
- Автоматически освобождает при выходе из области видимости
- Даже если произойдёт исключение — мьютекс освободится (RAII-принцип)

---

### 3. Синхронный режим работы с Named Pipe

**Что значит "синхронный"?**

Когда клиент вызывает `WriteFile()` или `ReadFile()`, программа **останавливается и ждёт** завершения операции:

```cpp
// Синхронный вызов — программа "замирает" пока данные не отправятся
WriteFile(hPipe, data, length, &bytesWritten, NULL);
// Следующая строка выполнится только после завершения записи

// То же с чтением — ждём пока сервер ответит
ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL);
```

**Почему не асинхронный?**

По требованиям:
- **Сервер** должен быть асинхронным (обслуживать много клиентов)
- **Клиент** может быть синхронным (проще в реализации)

В нашем случае синхронность не проблема, потому что:
- Каждый поток имеет **свой собственный** `PipeClient`
- Пока поток 1 ждёт ответа, потоки 2-8 продолжают работать
- Многопоточность компенсирует блокировку

---

### 4. Работа с памятью

**LinkedList — ручное управление памятью:**

```cpp
template<typename T>
class LinkedList {
    struct Node {
        T data;
        Node* next;
    };
    
    void push_back(const T& value) {
        Node* newNode = new Node(value);  // Выделяем память
        // ... добавляем в список
    }
    
    ~LinkedList() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;  // Освобождаем память
        }
    }
};
```

**Почему не `std::list`?**

По требованиям нужно реализовать **собственный** шаблонный класс однонаправленного списка.

**RAII — автоматическое освобождение ресурсов:**

```cpp
class PipeClient {
    HANDLE hPipe;
    
    ~PipeClient() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);  // Автоматически при уничтожении объекта
        }
    }
};

// Использование:
{
    PipeClient client;
    client.Connect();
    client.TryPassword("admin", "123");
}  // Здесь client уничтожается, дескриптор закрывается автоматически
```

---

### 5. Таймауты и измерение времени

**Высокоточный таймер:**

```cpp
class Timer {
    std::chrono::high_resolution_clock::time_point startTime;
    
    void Start() {
        startTime = std::chrono::high_resolution_clock::now();
    }
    
    unsigned long long GetElapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        return duration.count();  // Миллисекунды с момента старта
    }
};
```

**Проверка таймаута в атаке:**

```cpp
// Константа: 5 минут = 300000 миллисекунд
const unsigned long long PASSWORD_CRACKING_TIMEOUT_MS = 5 * 60 * 1000;

bool IsTimedOut() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    
    return elapsed >= PASSWORD_CRACKING_TIMEOUT_MS;  // Прошло больше 5 минут?
}
```

---

### 6. Обработка ошибок Named Pipe

**Типичные ошибки и как с ними справляемся:**

```cpp
bool PipeClient::Connect() {
    hPipe = CreateFileA(pipeName, ...);
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        
        if (error == ERROR_PIPE_BUSY) {
            // Сервер занят другим клиентом
            // Решение: подождать и попробовать снова
            WaitNamedPipeA(pipeName, 5000);  // Ждём до 5 секунд
            hPipe = CreateFileA(pipeName, ...);  // Пробуем снова
        }
        else if (error == ERROR_FILE_NOT_FOUND) {
            // Сервер не запущен
            std::cout << "Сервер не найден!\n";
            return false;
        }
    }
    return hPipe != INVALID_HANDLE_VALUE;
}
```

**При разрыве соединения:**

```cpp
bool ReceiveResponse(DWORD& response) {
    if (!ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL)) {
        DWORD error = GetLastError();
        
        if (error == ERROR_BROKEN_PIPE) {
            // Сервер закрыл соединение
            // Решение: переподключиться и попробовать снова
            if (Reconnect()) {
                return ReadFile(...);  // Повторная попытка
            }
        }
        return false;
    }
    return true;
}
```

---

### 7. Генерация паролей без рекурсии

**Почему нельзя рекурсию?**

По требованиям: "НЕ использовать рекурсию при взломе". Причины:
- Рекурсия создаёт глубокий стек вызовов
- При длинных паролях может быть переполнение стека
- Итеративный подход эффективнее

**Итеративный алгоритм (как одометр в машине):**

```cpp
// Алфавит: "abc", текущий пароль: "abc"
// Нужно получить следующий: "aaa" (с увеличением длины? нет, сначала "baa")

bool IncrementPassword() {
    int pos = currentPassword.length() - 1;  // Начинаем с конца
    
    while (pos >= 0) {
        // Находим текущий символ в алфавите
        size_t charIndex = alphabet.find(currentPassword[pos]);
        
        if (charIndex < alphabet.length() - 1) {
            // Можем увеличить этот символ
            currentPassword[pos] = alphabet[charIndex + 1];
            return true;  // Готово: "abc" → "abd"
        }
        
        // Символ на максимуме — сбрасываем и переходим влево
        currentPassword[pos] = alphabet[0];  // 'c' → 'a'
        pos--;
    }
    
    // Все символы переполнились — увеличиваем длину
    if (currentPassword.length() < maxLength) {
        currentPassword = std::string(currentPassword.length() + 1, alphabet[0]);
        return true;  // "zzz" → "aaaa"
    }
    
    return false;  // Все комбинации перебраны
}
```

**Пример последовательности для алфавита "ab":**
```
a → b → aa → ab → ba → bb → aaa → aab → aba → abb → baa → ...
```

---

### 8. Дедупликация в атаке по словарю

**Проблема:** Разные правила могут генерировать одинаковые пароли:
- `CASE_VARIATIONS("Hello")` → `hello`
- `DIGIT_SUFFIXES("hello")` → `hello` (оригинал тоже добавляется)

**Решение: два контейнера**

```cpp
LinkedList<std::string> variants;           // Для перебора по порядку
std::unordered_set<std::string> variantSet; // Для быстрой проверки дубликатов

void AddVariant(const std::string& variant) {
    // O(1) проверка — есть ли уже такой пароль?
    if (variantSet.find(variant) == variantSet.end()) {
        variantSet.insert(variant);      // Добавляем в set
        variants.push_back(variant);     // Добавляем в список
    }
    // Если уже есть — просто игнорируем
}
```

**Почему два контейнера?**
- `unordered_set` — O(1) поиск, но нет порядка
- `LinkedList` — O(n) поиск, но сохраняет порядок добавления

Нам нужно и то, и другое: быстрая проверка + последовательный перебор.

---

### 9. Цветной вывод в консоль

**Как это работает в Windows:**

```cpp
void SetColor(Color foreground) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);  // Дескриптор консоли
    SetConsoleTextAttribute(handle, foreground);      // Устанавливаем цвет
}

void PrintSuccess(const std::string& message) {
    SetColor(LIGHT_GREEN);           // Зелёный
    std::cout << "[+] " << message;
    SetColor(LIGHT_GRAY);            // Обратно серый
    std::cout << "\n";
}

// Результат в консоли:
// [+] PASSWORD FOUND!  ← зелёным цветом
```

---

### 10. Диалог выбора файла

**Стандартный Windows-диалог:**

```cpp
bool LoadDictionaryFromFile() {
    OPENFILENAMEA ofn = {};                          // Структура настроек
    char fileName[MAX_PATH] = "";                    // Буфер для пути
    
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0"; // Фильтр: только .txt
    ofn.lpstrTitle = "Select Dictionary File";       // Заголовок окна
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        // Пользователь выбрал файл
        // fileName теперь содержит полный путь, например:
        // "C:\Users\User\Documents\passwords.txt"
        return LoadDictionaryFromFile(fileName);
    }
    
    return false;  // Пользователь нажал "Отмена"
}
```

---

*Документ создан на основе анализа исходного кода клиентского приложения.*
*Версия: 1.0*
*Дата: Декабрь 2024*
