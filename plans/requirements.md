# Laboratory Work №1: Password Security & Cracking Simulation

## 1. Objective
Study recommendations for password formation and methods of password cracking using **Brute-force**, **Dictionary attacks**, and **Rule-based attacks**. Develop software to simulate these attacks using **WinAPI Named Pipes**.

---

## 2. General Organization
* **Format:** Group of 2 students.
* **Roles:**
    1.  **Administrator (Server):** Sets logins/passwords, manages the server, implements protection.
    2.  **Hacker (Client):** Attempts to crack passwords knowing only the login.
* **Architecture:** Client-Server application using **Named Pipes** (IPC).
* **Tech Stack:** C++ (recommended due to WinAPI usage), Object-Oriented Programming (OOP) required.

---

## 3. Application Requirements

### A. The Server Application (Administrator)
**Responsibilities:**
1.  **Configuration:**
    * On startup, ask for a text file via `GetOpenFileName`.
    * **File Format:**
        * Line 1: Max Login Length (space) Max Password Length.
        * Subsequent Lines: Login (space) Password (5 pairs total).
2.  **Setup:**
    * Select Mode: **Normal** or **With Anti-Cracking Protection** (see Grading 100%).
    * Create at least **3 instances** of a Named Pipe (Async/Overlapped mode recommended).
    * Pipe name must be consistent between Server and Client.
3.  **Operation:**
    * Wait for client connections.
    * Read `Login` and `Password` strings from the client.
    * **Validation:**
        * If match: Send `1` (Success).
        * If mismatch: Send `0` (Failure).
    * Disconnect the client immediately after the response.
    * Repeat listening process.
4.  **Anti-Cracking Logic (Protection Mode):**
    * Implement an artificial delay (or request skipping) for a specific login after repeated failures.
    * **Constraint:** This delay must **not** freeze the whole server; other clients must still be served instantly.

### B. The Client Application (Hacker)
**Responsibilities:**
1.  **Initial Setup:**
    * Select Mode: **Connection Test** or **Hacking Mode**.
    * **Connection Test:** User manually enters Login/Pass to verify server connectivity.
    * **Hacking Mode:** Select attack type (Brute-force or Rule-based).
2.  **Brute-Force Configuration:**
    * Select Alphabet:
        * Lower Latin + apostrophe (27 chars).
        * Lower/Upper Latin + apostrophe + digits (63 chars).
        * All above + Cyrillic (129 chars).
    * Input max password length.
    * Input target Login.
3.  **Rule-Based Attack Configuration (100% Grade):**
    * Load "Probable Passwords" from a file via `GetOpenFileName`.
    * Apply heuristic rules (implemented as class methods):
        * Reverse string.
        * Change case.
        * Keyboard layout swap (e.g., typing "ghbdtn" instead of "привет").
        * Transposition of characters.
    * Data Structure: Use a **Templated Singly Linked List** to store password dictionaries and rule sets.
4.  **Process:**
    * Connect to the Server's Named Pipe.
    * If Server is busy -> Wait/Retry.
    * Send Login + Candidate Password.
    * Receive Response (0 or 1).
    * **Important:** Recursion is forbidden for the cracking loop.
5.  **Output (Hacking Mode):**
    * Display current candidate password.
    * Display elapsed time.
    * Upon success: Show the cracked password.

---

## 4. Grading Criteria & Features

| Grade Level | Client Features | Server Features |
| :--- | :--- | :--- |
| **75%** | Brute-force attack. | Basic handling of connections. |
| **100%** | + Rule-based attack.<br>+ Use "Probable Password" lists.<br>+ Heuristic rules class. | + **Anti-cracking mode** (Delay logic). |
| **140%** | + **Multithreading** (Client generates attacks in threads). | + **Multithreading** (Handle multiple clients concurrently).<br>+ Bonus if >60% passwords cracked. |

---

## 5. The Scenario (Data)
The Administrator creates 5 passwords.
* **Known:** Hacker knows 1 password (for testing connectivity).
* **Unknown:** 4 passwords are unknown.
* **Password Characteristics:**
    1.  Violates "Don't use common sequences" (e.g., "123", "qwerty").
    2.  Violates "Don't use personal info" (dates, names).
    3.  Violates "Don't use short passwords".
    4.  Strong password (follows all rules).

---

## 6. Implementation Details (WinAPI)
* **Headers:** `windows.h`, `winbase.h`.
* **Server Functions:**
    * `CreateNamedPipe`: Use `FILE_FLAG_OVERLAPPED` for async.
    * `ConnectNamedPipe`: Listen for clients.
    * `DisconnectNamedPipe`: Reset pipe for next client.
* **Client Functions:**
    * `CreateFile` or `CallNamedPipe`: To connect.
    * `WaitNamedPipe`: To wait if server is busy.
    * `SetNamedPipeHandleState`: To switch between byte/message modes.
* **I/O:** `ReadFile`, `WriteFile`.

---

## 7. Report Requirements
The final report must include:
1.  **Table of Logins/Passwords:** Indicate which security recommendation was violated.
2.  **Timing Data:** Time taken to crack each password.
3.  **Success Metrics:** Number of cracked passwords vs. server mode.
4.  **Graphs:** Time vs. Alphabet Size / Length / Attack Type.
5.  **Hardware Specs:** CPU threads, RAM, Network speed.
6.  **Analysis:** Conclusions on the effectiveness of attacks vs. protection.