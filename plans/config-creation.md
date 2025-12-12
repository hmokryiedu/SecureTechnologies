# Задание: Создание конфигурационного файла password_crack_mapping.json

## 📋 Цель

Создать конфигурационный файл `password_crack_mapping.json`, который:
- Классифицирует каждый пароль из `passwords.txt` по типам нарушений безопасности
- Определяет оптимальный метод атаки (rule-based или brute-force) для каждого пароля
- Содержит метаинформацию для генерации лабораторного отчета (Section 7 из requirements.md)
- Помогает анализировать эффективность различных методов крекинга

---

## 🎯 Связь с requirements.md

**Section 7 - Report Requirements:**
> "Table of Logins/Passwords: Indicate which security recommendation was violated."
> "Success Metrics: Number of cracked passwords vs. server mode."
> "Graphs: Time vs. Alphabet Size / Length / Attack Type."

**Этот конфиг позволит:**
- ✅ Автоматически генерировать таблицу нарушений
- ✅ Сравнивать ожидаемые vs реальные результаты
- ✅ Анализировать эффективность каждого метода
- ✅ Оптимизировать порядок атак

---

## 📂 Расположение файла

```
brute-force/
├── password_crack_mapping.json  ← НОВЫЙ ФАЙЛ
├── passwords.txt
├── vocabularies.json
└── crack_automation.py
```

---

## 📐 Структура JSON

### Общая структура:

```json
{
  "metadata": {
    "version": "1.0",
    "created": "2025-12-12",
    "description": "Password to crack mapping and security violation analysis",
    "total_passwords_in_file": 144
  },

  "password_crack_mapping": {
    "description": "Which attack method should crack which password",
    "passwords": [
      { /* Пароль 1 */ },
      { /* Пароль 2 */ },
      ...
    ],
    "summary": { /* Итоги */ }
  }
}
```

---

## 🔍 Структура каждого пароля в массиве

```json
{
  "id": 1,
  "login": "david",
  "password": "qwer",
  "length": 4,
  "character_set": "lowercase",

  "violation_analysis": {
    "violation_category": "Common keyboard layout sequence",
    "violated_rules": [
      "Don't use common sequences"
    ],
    "security_recommendation": "QWERTY-like patterns are among the first to be tried",
    "why_vulnerable": "The password is the keyboard pattern starting from 'Q'"
  },

  "attack_strategy": {
    "primary_attack": "rule_based",
    "secondary_attack": "brute_force",
    "recommended_vocabulary": "vocab_common_sequences.txt",
    "transformation_rules": [
      "case_variations",
      "reverse"
    ],
    "expected_crack_time_ms": 150,
    "confidence_percent": 95
  },

  "brute_force_analysis": {
    "alphabet_id": 1,
    "alphabet_name": "lowercase",
    "alphabet_size": 26,
    "max_length": 4,
    "total_combinations": 456976,
    "position_in_search_space_percent": 0.25
  },

  "test_notes": "Keyboard layout patterns (QWERTY, ASDF, ZXCV) are in common_sequences"
}
```

---

## 📝 Классификация паролей по violation_category

Используй следующие категории (из requirements.md Section 5):

### 1. "Common keyboard sequences"
- Примеры: `qwer`, `asdf`, `zxcv`, `qwertyui`, `asdfghjk`
- Словарь: `vocab_common_sequences.txt`
- Первичная атака: **rule_based**
- Вторичная атака: **brute_force** (alphabet_id: 1)

### 2. "Most common passwords"
- Примеры: `password`, `admin`, `123456`, `welcome`
- Словарь: `vocab_common_sequences.txt`
- Первичная атака: **rule_based**
- Вторичная атака: **brute_force** (alphabet_id: 1-3)

### 3. "Personal information patterns"
- Примеры: `Anna2000`, `Ivan1995`, `Maks2010`
- Словарь: `vocab_personal_info.txt`
- Трансформации: `case_variations`, `digit_suffixes`
- Первичная атака: **rule_based**
- Вторичная атака: **brute_force** (alphabet_id: 3)

### 4. "Too short passwords"
- Примеры: `a`, `ab`, `abc`, `qwe`, `Xy`
- Словарь: `vocab_short_words.txt`
- Первичная атака: **rule_based** или **brute_force**
- Первичная атака: **brute_force** (alphabet_id: 1)

### 5. "Cyrillic/transliteration patterns"
- Примеры: пароли, которые могут быть латиницей или кириллицей
- Словарь: `vocab_russian_cyrillic.txt`
- Трансформации: `keyboard_layout_swap`, `cyrillic_to_latin`
- Первичная атака: **rule_based**
- Вторичная атака: **brute_force** (alphabet_id: 3)

### 6. "Mixed case + digits + special"
- Примеры: `Qwerty1!`, `Pass123!`, `Abc123!`
- Словарь: `vocab_common_sequences.txt` + трансформации
- Трансформации: `case_variations`, `digit_suffixes`
- Первичная атака: **rule_based**
- Вторичная атака: **brute_force** (alphabet_id: 3)

### 7. "Strong password - no obvious pattern"
- Примеры: случайная комбинация без очевидного паттерна
- Словарь: Нет
- Первичная атака: **brute_force**
- Вторичная атака: Нет

---

## 🔄 Трансформационные правила

Для каждого пароля указать, какие правила применяются:

```json
"transformation_rules": [
  "case_variations",           // lowercase, UPPERCASE, Capitalize
  "reverse",                   // обратный порядок
  "digit_suffixes",           // добавить 1, 12, 123, 2024, и т.д.
  "keyboard_layout_swap",     // латынь ↔ кириллица
  "character_transposition",  // переставить символы
  "none"                      // прямое совпадение без трансформаций
]
```

---

## 📊 Раздел summary

```json
"summary": {
  "total_passwords": 144,
  "classification": {
    "common_sequences": 45,
    "personal_information": 30,
    "cyrillic_patterns": 20,
    "short_passwords": 15,
    "mixed_case_digits_special": 25,
    "strong_passwords": 9
  },
  "attack_distribution": {
    "rule_based_primary": 120,
    "brute_force_primary": 24
  },
  "success_rate_analysis": {
    "expected_success_with_rule_based_percent": 83,
    "expected_success_with_brute_force_percent": 100,
    "brute_force_only_passwords": 24,
    "rule_based_faster_passwords": 110
  },
  "time_analysis": {
    "average_expected_time_ms_rule_based": 250,
    "average_expected_time_ms_brute_force": 5000,
    "fastest_crack_ms": 10,
    "slowest_crack_ms": 30000
  }
}
```

---

## 📝 Инструкции по заполнению

### 1. Анализ каждого пароля из passwords.txt

Для каждой пары (login, password):

```bash
Шаг 1: Определить длину пароля
  "length": len(password)

Шаг 2: Определить character_set
  - "lowercase"           (a-z only)
  - "uppercase"           (A-Z only)
  - "mixed_case"          (a-z, A-Z)
  - "with_digits"         (+ 0-9)
  - "with_special"        (+ !@#$%^&* и т.д.)
  - "cyrillic"           (кириллица)
  - "mixed_scripts"      (латынь + кириллица)

Шаг 3: Найти в словарях
  - Ищется ли password в vocab_common_sequences.txt?
  - Ищется ли password в vocab_personal_info.txt?
  - Ищется ли password в vocab_russian_cyrillic.txt?
  - Ищется ли password в vocab_short_words.txt?

Шаг 4: Определить необходимые трансформации
  - Нужны ли case variations?
  - Нужны ли digit suffixes?
  - Нужна ли транслитерация?
  - Нужны ли перестановки?

Шаг 5: Определить primary_attack
  - Если находится в одном из словарей → rule_based
  - Если короткий и не в словарях → brute_force
  - Если только с трансформациями → rule_based

Шаг 6: Определить expected_crack_time_ms
  - Если найдено прямо в словаре: 50-200 мс
  - Если нужны трансформации: 200-500 мс
  - Если нужен brute_force (short): 10-100 мс
  - Если нужен brute_force (longer): 1000-30000 мс
```

### 2. Примеры анализа

#### Пример 1: "qwer"
```json
{
  "login": "david",
  "password": "qwer",
  "violation_category": "Common keyboard layout sequence",
  "primary_attack": "rule_based",
  "recommended_vocabulary": "vocab_common_sequences.txt",
  "transformation_rules": ["none"],
  "expected_crack_time_ms": 50,
  "confidence_percent": 99
}
```

#### Пример 2: "Anna2000"
```json
{
  "login": "maria_garcia",
  "password": "Anna2000",
  "violation_category": "Personal information (name + birth year)",
  "primary_attack": "rule_based",
  "recommended_vocabulary": "vocab_personal_info.txt",
  "transformation_rules": ["case_variations", "digit_suffixes"],
  "expected_crack_time_ms": 300,
  "confidence_percent": 90
}
```

#### Пример 3: "abc"
```json
{
  "login": "michael_brown",
  "password": "abc",
  "violation_category": "Too short password",
  "primary_attack": "rule_based",
  "recommended_vocabulary": "vocab_short_words.txt",
  "transformation_rules": ["none"],
  "expected_crack_time_ms": 30,
  "confidence_percent": 98
}
```

#### Пример 4: "mwfqxnpvgkjh"
```json
{
  "login": "david_jones",
  "password": "mwfqxnpvgkjh",
  "violation_category": "No obvious pattern - strong password",
  "primary_attack": "brute_force",
  "recommended_vocabulary": null,
  "transformation_rules": ["none"],
  "alphabet_id": 1,
  "expected_crack_time_ms": 20000,
  "confidence_percent": 0
}
```

---

## 🛠️ Инструменты для создания

### Вариант 1: Ручное создание (Python скрипт)

```python
# analyze_passwords.py
import json
from pathlib import Path

class PasswordAnalyzer:
    def __init__(self):
        self.passwords = self._load_passwords()
        self.vocabularies = self._load_vocabularies()
        self.mapping = []

    def _load_passwords(self):
        """Загрузить пароли из passwords.txt"""
        with open('passwords.txt', 'r') as f:
            lines = f.readlines()
            passwords = []
            for line in lines[1:]:  # Пропустить заголовок
                parts = line.strip().split()
                if len(parts) >= 2:
                    passwords.append({
                        'login': parts[0],
                        'password': parts[1]
                    })
            return passwords

    def _load_vocabularies(self):
        """Загрузить словари"""
        vocabs = {}
        for vocab_file in [
            'vocab_common_sequences.txt',
            'vocab_personal_info.txt',
            'vocab_russian_cyrillic.txt',
            'vocab_short_words.txt'
        ]:
            with open(f'vocabularies/{vocab_file}', 'r') as f:
                vocabs[vocab_file] = set(line.strip() for line in f)
        return vocabs

    def analyze_password(self, login: str, password: str) -> dict:
        """Анализировать один пароль"""
        # TODO: Реализовать анализ
        # Проверить в словарях
        # Определить violation_category
        # Определить attack strategy
        pass

    def generate_mapping(self):
        """Генерировать полный mapping"""
        result = {
            "metadata": {
                "version": "1.0",
                "total_passwords": len(self.passwords)
            },
            "password_crack_mapping": {
                "passwords": [],
                "summary": {}
            }
        }

        for pwd in self.passwords:
            analysis = self.analyze_password(pwd['login'], pwd['password'])
            result['password_crack_mapping']['passwords'].append(analysis)

        # Заполнить summary
        result['password_crack_mapping']['summary'] = self._generate_summary(
            result['password_crack_mapping']['passwords']
        )

        return result

    def _generate_summary(self, passwords: list) -> dict:
        """Генерировать статистику"""
        # TODO: Реализовать
        pass

    def save_json(self, output_file: str):
        """Сохранить в JSON"""
        mapping = self.generate_mapping()
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(mapping, f, indent=2, ensure_ascii=False)

if __name__ == '__main__':
    analyzer = PasswordAnalyzer()
    analyzer.save_json('password_crack_mapping.json')
```

### Вариант 2: Интерактивное создание (GUI)

Использовать интерфейс для:
- Просмотра пароля
- Выбора vocabulary
- Выбора transformation rules
- Установления expected_time

---

## ✅ Критерии готовности

Конфиг считается готовым, если:

- ✅ Содержит **все 144 пароля** из `passwords.txt`
- ✅ Каждый пароль имеет полную информацию:
  - `violation_category`
  - `primary_attack` (rule_based или brute_force)
  - `recommended_vocabulary` (если rule_based)
  - `transformation_rules`
  - `expected_crack_time_ms`
  - `confidence_percent`
- ✅ Для каждого пароля указан `alphabet_id` (1, 2 или 3)
- ✅ Раздел `summary` содержит правильные статистики
- ✅ JSON валидный (можно проверить через `python -m json.tool`)
- ✅ Расчеты confidence и expected_time реалистичны

---

## 📋 Чек-лист

- [ ] Создан файл `password_crack_mapping.json`
- [ ] Загруженные пароли из `passwords.txt` (все 144)
- [ ] Классифицированы по `violation_category`
- [ ] Определены `primary_attack` для каждого
- [ ] Заполнены `transformation_rules`
- [ ] Указаны `expected_crack_time_ms`
- [ ] Заполнен раздел `summary`
- [ ] JSON валидирован
- [ ] Протестирован скрипт загрузки конфига
- [ ] Документирован в README или комментариях

---

## 🎯 Дополнительно

### Использование в crack_automation.py

```python
def load_password_mapping(self):
    """Загрузить информацию о паролях"""
    with open('password_crack_mapping.json', 'r') as f:
        self.password_mapping = json.load(f)

def get_optimal_attack_order(self, login: str) -> list:
    """Получить оптимальный порядок атак для пароля"""
    for pwd_info in self.password_mapping['password_crack_mapping']['passwords']:
        if pwd_info['login'] == login:
            return [
                pwd_info['attack_strategy']['primary_attack'],
                pwd_info['attack_strategy']['secondary_attack']
            ]
```

### Для отчета

```python
def generate_report_table(self):
    """Генерировать таблицу для отчета"""
    mapping = self.password_mapping['password_crack_mapping']

    print("| Login | Password | Violation | Primary Attack | Expected Time (ms) | Actual Time (ms) | Status |")
    for pwd in mapping['passwords']:
        # Вывести строку таблицы
```

---

## 📚 Ссылки

- Требования: `plans/requirements.md` (Section 5 и 7)
- Словари: `vocabularies.json` (если создан)
- Пароли: `passwords.txt`

