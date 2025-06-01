#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <windows.h>
#include <winhttp.h>
#include <conio.h>
#include <vector>
#include "json.hpp"

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;
using namespace std;

enum ConsoleColor {
    BLACK = 0, BLUE = 1, GREEN = 2, CYAN = 3, RED = 4, MAGENTA = 5,
    BROWN = 6, LIGHTGRAY = 7, DARKGRAY = 8, LIGHTBLUE = 9, LIGHTGREEN = 10,
    LIGHTCYAN = 11, LIGHTRED = 12, LIGHTMAGENTA = 13, YELLOW = 14, WHITE = 15
};

struct Config {
    string discord_token;
    string guild_id;
    string channel_id;
};

struct ParsedLinkInfo {
    enum class LinkType { NONE, OLD_STYLE, NEW_SHARE_STYLE };
    LinkType type = LinkType::NONE;
    string code;
    string originalUrl;
};

void setConsoleColor(ConsoleColor text, ConsoleColor background = BLACK) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (background << 4) | text);
}

void resetConsoleColor() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (BLACK << 4) | LIGHTGRAY);
}

void displayLogo() {
    system("cls");
    setConsoleColor(CYAN);
    cout << R"(
   ____                 _             _       _                 
  / __ \               | |           | |     (_)                
 | |  | |_ __ __ _  ___| | ___       | | ___  _ _ __   ___ _ __ 
 | |  | | '__/ _` |/ __| |/ _ \  _   | |/ _ \| | '_ \ / _ \ '__|
 | |__| | | | (_| | (__| |  __/ | |__| | (_) | | | | |  __/ |   
  \____/|_|  \__,_|\___|_|\___|  \____/ \___/|_|_| |_|\___|_|   Public Version

)" << endl;
    setConsoleColor(YELLOW);
    cout << "                      Made by Oracle - v1.0.2" << endl << endl;
    resetConsoleColor();
}

void drawBox(int width, int height, int startX, int startY) {
    COORD coord;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    coord.X = static_cast<SHORT>(startX);
    coord.Y = static_cast<SHORT>(startY);
    SetConsoleCursorPosition(hConsole, coord);
    cout << "+" << string(width - 2, '-') << "+";
    for (int i = 1; i < height - 1; i++) {
        coord.X = static_cast<SHORT>(startX);
        coord.Y = static_cast<SHORT>(startY + i);
        SetConsoleCursorPosition(hConsole, coord);
        cout << "|" << string(width - 2, ' ') << "|";
    }
    coord.X = static_cast<SHORT>(startX);
    coord.Y = static_cast<SHORT>(startY + height - 1);
    SetConsoleCursorPosition(hConsole, coord);
    cout << "+" << string(width - 2, '-') << "+";
}

void drawPromptBox(const string& title, const string& message) {
    system("cls");
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = static_cast<int>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    int height = static_cast<int>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    int boxWidth = 60;
    if (boxWidth > width - 4) {
        boxWidth = width - 4;
    }
    int boxHeight = 7;
    int startX = (width - boxWidth) / 2;
    int startY = (height - boxHeight) / 2;
    drawBox(boxWidth, boxHeight, startX, startY);
    COORD coord;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    coord.X = static_cast<SHORT>(startX + (boxWidth - static_cast<int>(title.length())) / 2);
    coord.Y = static_cast<SHORT>(startY + 1);
    SetConsoleCursorPosition(hConsole, coord);
    setConsoleColor(LIGHTCYAN);
    cout << title;
    resetConsoleColor();
    coord.X = static_cast<SHORT>(startX + 2);
    coord.Y = static_cast<SHORT>(startY + 3);
    SetConsoleCursorPosition(hConsole, coord);
    cout << message;
    coord.X = static_cast<SHORT>(startX + (boxWidth - 35) / 2);
    coord.Y = static_cast<SHORT>(startY + 5);
    SetConsoleCursorPosition(hConsole, coord);
    setConsoleColor(LIGHTGREEN);
    cout << "Press ";
    setConsoleColor(WHITE);
    cout << "[";
    setConsoleColor(LIGHTGREEN);
    cout << "Y";
    setConsoleColor(WHITE);
    cout << "] to Join or [";
    setConsoleColor(LIGHTRED);
    cout << "N";
    setConsoleColor(WHITE);
    cout << "] to Skip";
    resetConsoleColor();
    coord.X = 0;
    coord.Y = static_cast<SHORT>(startY + boxHeight + 1);
    SetConsoleCursorPosition(hConsole, coord);
}

bool promptJoinServer(const string& link) {
    drawPromptBox("NEW PRIVATE SERVER DETECTED", "Link: " + link);
    char choice;
    do {
        choice = _getch();
        choice = static_cast<char>(tolower(choice));
    } while (choice != 'y' && choice != 'n');
    system("cls");
    displayLogo();
    if (choice == 'y') {
        setConsoleColor(LIGHTGREEN);
        cout << "[+] User chose to join the server." << endl;
        resetConsoleColor();
        return true;
    }
    else {
        setConsoleColor(LIGHTRED);
        cout << "[!] User chose to skip this server." << endl;
        resetConsoleColor();
        return false;
    }
}

void showSpinner(const string& message, int duration_ms) {
    const string spinChars = "|/-\\";
    int spinIndex = 0;
    auto start = chrono::high_resolution_clock::now();
    auto end = start + chrono::milliseconds(duration_ms);
    while (chrono::high_resolution_clock::now() < end) {
        cout << "\r" << message << " " << spinChars[spinIndex] << "    " << flush;
        spinIndex = (spinIndex + 1) % static_cast<int>(spinChars.length());
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    cout << "\r" << message << " Done    " << endl;
}

bool loadConfig(Config& config) {
    try {
        ifstream configFile("config.json");
        if (!configFile.is_open()) {
            setConsoleColor(RED);
            cerr << "Failed to open config.json" << endl;
            resetConsoleColor();
            return false;
        }
        json j;
        configFile >> j;
        config.discord_token = j["discord_token"];
        config.guild_id = j["guild_id"];
        config.channel_id = j["channel_id"];
        return true;
    }
    catch (const exception& e) {
        setConsoleColor(RED);
        cerr << "Error loading config: " << e.what() << endl;
        resetConsoleColor();
        return false;
    }
}

ParsedLinkInfo parseRobloxLink(const string& content) {
    ParsedLinkInfo linkInfo;
    smatch match;

    regex robloxRegexOld(R"(https?://(?:www\.)?roblox\.com/games/\d+/[^?]+\?privateServerLinkCode=([a-zA-Z0-9_-]+))");
    if (regex_search(content, match, robloxRegexOld) && match.size() > 1) {
        linkInfo.type = ParsedLinkInfo::LinkType::OLD_STYLE;
        linkInfo.originalUrl = match[0].str();
        linkInfo.code = match[1].str();
        return linkInfo;
    }

    regex robloxRegexNew(R"(https?://(?:www\.)?roblox\.com/share\?code=([a-zA-Z0-9]+)&type=Server)");
    if (regex_search(content, match, robloxRegexNew) && match.size() > 1) {
        linkInfo.type = ParsedLinkInfo::LinkType::NEW_SHARE_STYLE;
        linkInfo.originalUrl = match[0].str();
        linkInfo.code = match[1].str();
        return linkInfo;
    }

    return linkInfo;
}

void launchRoblox(const ParsedLinkInfo& linkInfo) {
    if (linkInfo.type == ParsedLinkInfo::LinkType::NONE) {
        setConsoleColor(RED);
        cerr << "[!] Invalid or unsupported link." << endl;
        resetConsoleColor();
        return;
    }
    string url = "roblox://navigation/share_links?code=" + linkInfo.code + "&type=Server";
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

string getLastMessageFromChannel(const Config& config) {
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL bResults = FALSE;
    static HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL, hRequest = NULL;
    string responseData;

    if (hSession == NULL) {
        hSession = WinHttpOpen(L"Oracle Roblox Joiner/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession == NULL) {
            setConsoleColor(RED);
            cerr << "Failed to initialize HTTP session. Error: " << GetLastError() << endl;
            resetConsoleColor();
            return "";
        }
        DWORD timeout = 5000;
        WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);
    }

    hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect == NULL) {
        setConsoleColor(RED);
        cerr << "Failed to connect to Discord. Error: " << GetLastError() << endl;
        resetConsoleColor();
        return "";
    }

    wstring path = L"/api/v10/channels/" + wstring(config.channel_id.begin(), config.channel_id.end()) + L"/messages?limit=1";
    hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest == NULL) {
        WinHttpCloseHandle(hConnect);
        setConsoleColor(RED);
        cerr << "Failed to create HTTP request. Error: " << GetLastError() << endl;
        resetConsoleColor();
        return "";
    }

    wstring authHeader = L"Authorization: " + wstring(config.discord_token.begin(), config.discord_token.end());
    wstring contentTypeHeader = L"Content-Type: application/json";
    wstring userAgentHeader = L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36";
    WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, contentTypeHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, userAgentHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

    bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!bResults) {
        DWORD error = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        setConsoleColor(RED);
        cerr << "Failed to send HTTP request. Error: " << error << endl;
        resetConsoleColor();
        return "";
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
        DWORD error = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        setConsoleColor(RED);
        cerr << "Failed to receive HTTP response. Error: " << error << endl;
        resetConsoleColor();
        return "";
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusCodeSize, NULL);
    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        setConsoleColor(RED);
        cerr << "HTTP Error: " << statusCode << endl;
        if (statusCode == 401 || statusCode == 403) {
            cerr << "Authentication failed. Please check your Discord token." << endl;
            cerr << "Make sure you're using a valid user token, not a bot token." << endl;
        }
        resetConsoleColor();
        return "";
    }

    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        pszOutBuffer = new char[dwSize + 1];
        if (!pszOutBuffer) break;
        ZeroMemory(pszOutBuffer, dwSize + 1);
        if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
            delete[] pszOutBuffer;
            break;
        }
        responseData.append(pszOutBuffer, dwDownloaded);
        delete[] pszOutBuffer;
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return responseData;
}

int main() {
    SetConsoleTitle(L"Oracle's Roblox Auto Joiner");
    displayLogo();
    setConsoleColor(LIGHTCYAN);
    cout << "Press ENTER to start the bot..." << endl;
    resetConsoleColor();
    _getch();
    setConsoleColor(YELLOW);
    showSpinner("Initializing Discord connection...", 1000);
    resetConsoleColor();
    Config config;
    if (!loadConfig(config)) {
        setConsoleColor(RED);
        cerr << "Failed to load configuration. Exiting." << endl;
        resetConsoleColor();
        cout << "Press any key to exit..." << endl;
        _getch();
        return 1;
    }
    if (config.discord_token.rfind("Bot ", 0) == 0) {
        setConsoleColor(RED);
        cerr << "Error: You're using a Bot token. This program requires a User token." << endl;
        cerr << "Please replace your token in config.json with a valid User token." << endl;
        resetConsoleColor();
        cout << "Press any key to exit..." << endl;
        _getch();
        return 1;
    }
    setConsoleColor(YELLOW);
    showSpinner("Loading WinHTTP services...", 500);
    resetConsoleColor();
    string testJson = getLastMessageFromChannel(config);
    if (testJson.empty()) {
        setConsoleColor(RED);
        cerr << "Failed to connect to Discord API. Please check your internet connection and token." << endl;
        resetConsoleColor();
        cout << "Press any key to exit..." << endl;
        _getch();
        return 1;
    }
    string lastMessageId = "";
    try {
        auto messages = json::parse(testJson);
        if (messages.is_array() && !messages.empty()) {
            lastMessageId = messages[0]["id"].get<string>();
            setConsoleColor(YELLOW);
            cout << "[!] Initialized with latest message ID: " << lastMessageId << endl;
            resetConsoleColor();
        }
    }
    catch (const exception& e) {
        setConsoleColor(RED);
        cerr << "Error initializing with last message: " << e.what() << endl;
        resetConsoleColor();
    }
    bool firstLinkFound = false;
    vector<string> detectedLinks;
    setConsoleColor(LIGHTGREEN);
    cout << "[+] Roblox Auto Link Joiner started successfully!" << endl;
    setConsoleColor(WHITE);
    cout << "[+] Monitoring channel " << config.channel_id << " for NEW Roblox private server links..." << endl;
    resetConsoleColor();
    int checkCount = 0;

    while (true) {
        string messagesJson = getLastMessageFromChannel(config);
        if (!messagesJson.empty()) {
            try {
                auto messages = json::parse(messagesJson);
                if (messages.is_array() && !messages.empty()) {
                    string currentMessageId = messages[0]["id"].get<string>();
                    if (currentMessageId != lastMessageId) {
                        lastMessageId = currentMessageId;
                        string messageContent = messages[0]["content"].get<string>();
                        ParsedLinkInfo parsedInfo = parseRobloxLink(messageContent);

                        if (parsedInfo.type != ParsedLinkInfo::LinkType::NONE) {
                            setConsoleColor(LIGHTCYAN);
                            cout << "[!] Roblox private server link detected!" << endl;
                            resetConsoleColor();
                            bool linkAlreadyDetected = false;
                            for (const auto& link : detectedLinks) {
                                if (link == parsedInfo.originalUrl) {
                                    linkAlreadyDetected = true;
                                    break;
                                }
                            }
                            if (linkAlreadyDetected) {
                                setConsoleColor(YELLOW);
                                cout << "[!] This link was already detected before. Ignoring." << endl;
                                resetConsoleColor();
                                continue;
                            }
                            detectedLinks.push_back(parsedInfo.originalUrl);
                            if (!firstLinkFound) {
                                firstLinkFound = true;
                                launchRoblox(parsedInfo);
                            }
                            else {
                                if (promptJoinServer(parsedInfo.originalUrl)) {
                                    launchRoblox(parsedInfo);
                                }
                            }
                        }
                    }
                }
            }
            catch (const exception& e) {
                setConsoleColor(RED);
                cerr << "Error processing messages: " << e.what() << endl;
                resetConsoleColor();
            }
        }
        checkCount++;
        if (checkCount % 100 == 0) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char timeStr[100];
            sprintf_s(timeStr, sizeof(timeStr), "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
            setConsoleColor(DARKGRAY);
            cout << "[" << timeStr << "] Still monitoring channel... (Press CTRL+C to exit)" << endl;
            resetConsoleColor();
            checkCount = 0;
        }
    }
    return 0;
}
