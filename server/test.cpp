#include <windows.h>
#include <string>
#include <thread>
#include "list.h"
#include "PipeServer.h"
#include "ServerContext.h"
#include "ServerConstants.h"

// Ідентифікатори для кнопок
#define IDC_LOAD_BTN 101
#define IDC_START_BTN 102
#define IDC_PROTECT_CHK 103
#define IDC_LOG_LIST 104

// Функція обробки повідомлень вікна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Retrieve ServerContext from window user data
    ServerContext* context = reinterpret_cast<ServerContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (uMsg) {
    case WM_CREATE:
        // Кнопка завантаження файлу
        CreateWindow("BUTTON", "Load Users File", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 10, 120, 30, hwnd, (HMENU)IDC_LOAD_BTN, NULL, NULL);

        // Чекбокс захисту
        CreateWindow("BUTTON", "Anti-Brute-Force Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            140, 10, 180, 30, hwnd, (HMENU)IDC_PROTECT_CHK, NULL, NULL);

        // Кнопка старту
        CreateWindow("BUTTON", "Start Server", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            330, 10, 100, 30, hwnd, (HMENU)IDC_START_BTN, NULL, NULL);

        // Список для логів
        CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", NULL, 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            10, 50, 460, 300, hwnd, (HMENU)IDC_LOG_LIST, NULL, NULL);
        break;

    case WM_COMMAND:
        if (context) {
            switch (LOWORD(wParam)) {
            case IDC_LOAD_BTN:
                if (context->GetUsers().Load(hwnd)) {
                    string msg = "Loaded " + to_string(context->GetUsers().Count()) + " users.";
                    SendDlgItemMessage(hwnd, IDC_LOG_LIST, LB_ADDSTRING, 0, (LPARAM)msg.c_str());
                }
                break;

            case IDC_PROTECT_CHK:
                {
                    BOOL checked = IsDlgButtonChecked(hwnd, IDC_PROTECT_CHK);
                    context->GetUsers().SetProtection(checked == BST_CHECKED);
                    SendDlgItemMessage(hwnd, IDC_LOG_LIST, LB_ADDSTRING, 0, 
                        (LPARAM)(checked ? "Protection ON" : "Protection OFF"));
                }
                break;

            case IDC_START_BTN:
                if (context->GetUsers().Count() == 0) {
                    MessageBox(hwnd, "Please load users file first!", "Error", MB_ICONERROR);
                } else {
                    unsigned int hwThreads = std::thread::hardware_concurrency() * 2;
                    MessageBox(hwnd, std::to_string(hwThreads).c_str(), "Info", MB_ICONINFORMATION);
                    context->GetServer().Start(hwThreads, &context->GetUsers(), hwnd);
                    EnableWindow(GetDlgItem(hwnd, IDC_START_BTN), FALSE); // Блокуємо кнопку
                }
                break;
            }
        }
        break;

    case WM_LOG_MSG: 
        {
            // Отримали лог від сервера (SendMessage - synchronous, no memory to free)
            const char* text = (const char*)wParam;
            LPARAM len = lParam;
            std::string logText(text, len);
            int idx = SendDlgItemMessage(hwnd, IDC_LOG_LIST, LB_ADDSTRING, 0, (LPARAM)logText.c_str());
            SendDlgItemMessage(hwnd, IDC_LOG_LIST, LB_SETTOPINDEX, idx, 0); // Прокрутка вниз
        }
        break;

    case WM_DESTROY:
        if (context) {
            context->GetServer().Stop();  // Graceful thread cleanup
            delete context;  // Clean up context
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Головна функція (замість main)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "LabServerClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(CLASS_NAME, "Password Server (Admin)", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    // Create and attach ServerContext to window
    ServerContext* context = new ServerContext();
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(context));

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}