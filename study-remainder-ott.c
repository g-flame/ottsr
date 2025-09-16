#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <psapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "psapi.lib")

// ENHANCED CONFIG STRUCTURE
typedef struct {
    char messages[100][512];
    int messageCount;
    char studyApps[50][512];
    int studyAppCount;
    char blockedApps[50][512];
    int blockedAppCount;
    int reminderInterval;
    int appCheckInterval;
    int enableFlash;
    int enableShake;
    int enableSound;
    int enableOpenGL;
    int enableAppMonitoring;
    int enableProductivityScore;
    int enableAchievements;
    int enableDarkMode;
    int soundVolume;
    int flashIntensity;
    int shakeIntensity;
    char subject[128];
    char motivationalQuotes[50][256];
    int quoteCount;
    int enableAutoLaunch;
    int enableBreakReminder;
    int breakInterval;
    int enableFocusMode;
    int enableHardcoreMode;
} Config;

// SESSION DATA STRUCTURE
typedef struct {
    time_t sessionStart;
    int studyMinutes;
    int distractionMinutes;
    int reminderCount;
    int achievementCount;
    float productivityScore;
    char distractingApps[100][128];
    int distractingAppTimes[100];
    int distractingAppCount;
    int streakDays;
    int totalSessions;
    int perfectSessions;
} SessionData;

// APP MONITORING STRUCTURE
typedef struct {
    char processName[128];
    DWORD processId;
    time_t startTime;
    int isStudyApp;
    int isBlocked;
} AppInfo;

// GUI CONTROLS IDS
#define IDC_SUBJECT_EDIT        1001
#define IDC_INTERVAL_EDIT       1002
#define IDC_FLASH_CHECK         1003
#define IDC_SHAKE_CHECK         1004
#define IDC_SOUND_CHECK         1005
#define IDC_OPENGL_CHECK        1006
#define IDC_APP_MONITOR_CHECK   1007
#define IDC_DARK_MODE_CHECK     1008
#define IDC_HARDCORE_CHECK      1009
#define IDC_FOCUS_MODE_CHECK    1010
#define IDC_START_BUTTON        1011
#define IDC_STOP_BUTTON         1012
#define IDC_STATS_BUTTON        1013
#define IDC_CONFIG_BUTTON       1014
#define IDC_STUDY_APPS_LIST     1015
#define IDC_BLOCKED_APPS_LIST   1016
#define IDC_ADD_STUDY_APP       1017
#define IDC_ADD_BLOCKED_APP     1018
#define IDC_VOLUME_SLIDER       1019
#define IDC_PRODUCTIVITY_BAR    1020
#define IDC_SESSION_TIME        1021
#define IDC_DISTRACTION_TIME    1022
#define IDC_STREAK_COUNTER      1023
#define IDC_ACHIEVEMENT_LIST    1024

// GLOBAL VARIABLES
Config config;
SessionData session;
AppInfo monitoredApps[200];
int monitoredAppCount = 0;
int isRunning = 0;
int isConfigOpen = 0;
HWND hwndMain, hwndConfig, hwndGL;
HWND hSubjectEdit, hIntervalEdit, hStudyAppsList, hBlockedAppsList;
HWND hProductivityBar, hSessionTime, hDistractionTime, hStreakCounter;
HANDLE hConsole;
HDC hdc;
HGLRC hrc;
float glowIntensity = 0.0f;
float particleSystem[200][6]; // x, y, vx, vy, life, color
HBRUSH darkBrush, accentBrush;
HFONT mainFont, boldFont;
time_t lastAppCheck = 0;
time_t sessionStartTime;

// ENHANCED PARTICLE SYSTEM
typedef struct {
    float x, y, vx, vy;
    float life, maxLife;
    float size;
    float r, g, b, a;
    int type; // 0=spark, 1=glow, 2=text
} Particle;

Particle particles[500];
int particleCount = 0;

// DEFAULT CONFIG CREATION
void createDefaultConfig() {
    FILE* file = fopen("study_reminder.conf", "w");
    if(!file) return;
    
    fprintf(file, "# ENHANCED UNIVERSAL STUDY REMINDER CONFIG v2.0\n");
    fprintf(file, "# MAXIMUM OVERKILL EDITION WITH GUI\n\n");
    
    fprintf(file, "SUBJECT=Dominating My Studies\n");
    fprintf(file, "REMINDER_INTERVAL=25\n");
    fprintf(file, "APP_CHECK_INTERVAL=10\n");
    fprintf(file, "BREAK_INTERVAL=5\n");
    fprintf(file, "ENABLE_FLASH=1\n");
    fprintf(file, "ENABLE_SHAKE=1\n");
    fprintf(file, "ENABLE_SOUND=1\n");
    fprintf(file, "ENABLE_OPENGL=1\n");
    fprintf(file, "ENABLE_APP_MONITORING=1\n");
    fprintf(file, "ENABLE_PRODUCTIVITY_SCORE=1\n");
    fprintf(file, "ENABLE_ACHIEVEMENTS=1\n");
    fprintf(file, "ENABLE_DARK_MODE=1\n");
    fprintf(file, "ENABLE_AUTO_LAUNCH=1\n");
    fprintf(file, "ENABLE_BREAK_REMINDER=1\n");
    fprintf(file, "ENABLE_FOCUS_MODE=0\n");
    fprintf(file, "ENABLE_HARDCORE_MODE=0\n");
    fprintf(file, "SOUND_VOLUME=75\n");
    fprintf(file, "FLASH_INTENSITY=50\n");
    fprintf(file, "SHAKE_INTENSITY=25\n\n");
    
    fprintf(file, "# EPIC MOTIVATIONAL MESSAGES\n");
    fprintf(file, "MESSAGES:\n");
    fprintf(file, "🔥 UNLEASH YOUR ACADEMIC BEAST MODE! 🔥\n");
    fprintf(file, "⚡ KNOWLEDGE THUNDERSTORM INCOMING! ⚡\n");
    fprintf(file, "🚀 BRAIN ROCKETS LAUNCHING TO SUCCESS! 🚀\n");
    fprintf(file, "💎 TRANSFORM INTO A LEARNING DIAMOND! 💎\n");
    fprintf(file, "🎯 PRECISION STRIKE ON IGNORANCE! 🎯\n");
    fprintf(file, "🔥 STUDY INFERNO: BURN THROUGH CONCEPTS! 🔥\n");
    fprintf(file, "⭐ COSMIC INTELLIGENCE LEVEL UP! ⭐\n");
    fprintf(file, "💪 MENTAL MUSCLES NEED WORKOUT TIME! 💪\n");
    fprintf(file, "🧠 NEURAL PATHWAYS DEMANDING UPGRADES! 🧠\n");
    fprintf(file, "🏆 CHAMPIONSHIP MINDSET ACTIVATED! 🏆\n");
    fprintf(file, "END_MESSAGES\n\n");
    
    fprintf(file, "# MOTIVATIONAL QUOTES FOR ACHIEVEMENTS\n");
    fprintf(file, "QUOTES:\n");
    fprintf(file, "The expert in anything was once a beginner.\n");
    fprintf(file, "Success is the sum of small efforts repeated day in and day out.\n");
    fprintf(file, "Don't watch the clock; do what it does. Keep going.\n");
    fprintf(file, "The future depends on what you do today.\n");
    fprintf(file, "Education is the most powerful weapon to change the world.\n");
    fprintf(file, "END_QUOTES\n\n");
    
    fprintf(file, "# STUDY APPLICATIONS (Auto-launch and track)\n");
    fprintf(file, "STUDY_APPS:\n");
    fprintf(file, "notepad++.exe\n");
    fprintf(file, "code.exe\n");
    fprintf(file, "devenv.exe\n");
    fprintf(file, "chrome.exe\n");
    fprintf(file, "firefox.exe\n");
    fprintf(file, "acrobat.exe\n");
    fprintf(file, "winword.exe\n");
    fprintf(file, "excel.exe\n");
    fprintf(file, "calc.exe\n");
    fprintf(file, "END_STUDY_APPS\n\n");
    
    fprintf(file, "# BLOCKED/DISTRACTION APPS (Monitor and warn)\n");
    fprintf(file, "BLOCKED_APPS:\n");
    fprintf(file, "steam.exe\n");
    fprintf(file, "discord.exe\n");
    fprintf(file, "spotify.exe\n");
    fprintf(file, "youtube.exe\n");
    fprintf(file, "instagram.exe\n");
    fprintf(file, "tiktok.exe\n");
    fprintf(file, "netflix.exe\n");
    fprintf(file, "twitch.exe\n");
    fprintf(file, "END_BLOCKED_APPS\n");
    
    fclose(file);
}

// ENHANCED CONFIG LOADER
void loadConfig() {
    FILE* file = fopen("study_reminder.conf", "r");
    if(!file) {
        createDefaultConfig();
        file = fopen("study_reminder.conf", "r");
    }
    
    // Set enhanced defaults
    strcpy(config.subject, "Dominating My Studies");
    config.reminderInterval = 25;
    config.appCheckInterval = 10;
    config.breakInterval = 5;
    config.enableFlash = 1;
    config.enableShake = 1;
    config.enableSound = 1;
    config.enableOpenGL = 1;
    config.enableAppMonitoring = 1;
    config.enableProductivityScore = 1;
    config.enableAchievements = 1;
    config.enableDarkMode = 1;
    config.enableAutoLaunch = 1;
    config.enableBreakReminder = 1;
    config.enableFocusMode = 0;
    config.enableHardcoreMode = 0;
    config.soundVolume = 75;
    config.flashIntensity = 50;
    config.shakeIntensity = 25;
    config.messageCount = 0;
    config.studyAppCount = 0;
    config.blockedAppCount = 0;
    config.quoteCount = 0;
    
    char line[512];
    int inMessages = 0, inStudyApps = 0, inBlockedApps = 0, inQuotes = 0;
    
    while(fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if(line[0] == '#' || strlen(line) == 0) continue;
        
        if(strncmp(line, "SUBJECT=", 8) == 0) {
            strcpy(config.subject, line + 8);
        }
        else if(strncmp(line, "REMINDER_INTERVAL=", 18) == 0) {
            config.reminderInterval = atoi(line + 18);
        }
        else if(strncmp(line, "APP_CHECK_INTERVAL=", 19) == 0) {
            config.appCheckInterval = atoi(line + 19);
        }
        else if(strncmp(line, "SOUND_VOLUME=", 13) == 0) {
            config.soundVolume = atoi(line + 13);
        }
        else if(strncmp(line, "ENABLE_HARDCORE_MODE=", 21) == 0) {
            config.enableHardcoreMode = atoi(line + 21);
        }
        // ... (other config parsing)
        else if(strcmp(line, "MESSAGES:") == 0) {
            inMessages = 1; inStudyApps = 0; inBlockedApps = 0; inQuotes = 0;
        }
        else if(strcmp(line, "STUDY_APPS:") == 0) {
            inStudyApps = 1; inMessages = 0; inBlockedApps = 0; inQuotes = 0;
        }
        else if(strcmp(line, "BLOCKED_APPS:") == 0) {
            inBlockedApps = 1; inMessages = 0; inStudyApps = 0; inQuotes = 0;
        }
        else if(strcmp(line, "QUOTES:") == 0) {
            inQuotes = 1; inMessages = 0; inStudyApps = 0; inBlockedApps = 0;
        }
        else if(strncmp(line, "END_", 4) == 0) {
            inMessages = inStudyApps = inBlockedApps = inQuotes = 0;
        }
        else if(inMessages && config.messageCount < 100) {
            strcpy(config.messages[config.messageCount], line);
            config.messageCount++;
        }
        else if(inStudyApps && config.studyAppCount < 50) {
            strcpy(config.studyApps[config.studyAppCount], line);
            config.studyAppCount++;
        }
        else if(inBlockedApps && config.blockedAppCount < 50) {
            strcpy(config.blockedApps[config.blockedAppCount], line);
            config.blockedAppCount++;
        }
        else if(inQuotes && config.quoteCount < 50) {
            strcpy(config.motivationalQuotes[config.quoteCount], line);
            config.quoteCount++;
        }
    }
    
    fclose(file);
}

// SESSION DATA MANAGEMENT
void loadSessionData() {
    FILE* file = fopen("session.txt", "r");
    if(!file) {
        memset(&session, 0, sizeof(SessionData));
        session.sessionStart = time(NULL);
        return;
    }
    
    fscanf(file, "%d %d %d %d %f %d %d %d", 
           &session.studyMinutes, &session.distractionMinutes,
           &session.reminderCount, &session.achievementCount,
           &session.productivityScore, &session.streakDays,
           &session.totalSessions, &session.perfectSessions);
    
    session.sessionStart = time(NULL);
    fclose(file);
}

void saveSessionData() {
    FILE* file = fopen("session.txt", "w");
    if(!file) return;
    
    fprintf(file, "%d %d %d %d %.2f %d %d %d\n", 
            session.studyMinutes, session.distractionMinutes,
            session.reminderCount, session.achievementCount,
            session.productivityScore, session.streakDays,
            session.totalSessions, session.perfectSessions);
    
    // Save detailed session log
    FILE* logFile = fopen("session_log.txt", "a");
    if(logFile) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(logFile, "[%04d-%02d-%02d %02d:%02d] Session: Study=%dm, Distract=%dm, Score=%.1f%%\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, session.studyMinutes, 
                session.distractionMinutes, session.productivityScore);
        fclose(logFile);
    }
    
    fclose(file);
}

// ENHANCED APP MONITORING
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if(!hProcess) return TRUE;
    
    char processName[MAX_PATH];
    if(GetModuleBaseName(hProcess, NULL, processName, MAX_PATH)) {
        // Check if window is visible and not minimized
        if(IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            // Add to monitored apps if not already present
            int found = 0;
            for(int i = 0; i < monitoredAppCount; i++) {
                if(monitoredApps[i].processId == processId) {
                    found = 1;
                    break;
                }
            }
            
            if(!found && monitoredAppCount < 200) {
                strcpy(monitoredApps[monitoredAppCount].processName, processName);
                monitoredApps[monitoredAppCount].processId = processId;
                monitoredApps[monitoredAppCount].startTime = time(NULL);
                
                // Check if it's a study app
                monitoredApps[monitoredAppCount].isStudyApp = 0;
                for(int j = 0; j < config.studyAppCount; j++) {
                    if(strstr(processName, config.studyApps[j])) {
                        monitoredApps[monitoredAppCount].isStudyApp = 1;
                        break;
                    }
                }
                
                // Check if it's a blocked app
                monitoredApps[monitoredAppCount].isBlocked = 0;
                for(int j = 0; j < config.blockedAppCount; j++) {
                    if(strstr(processName, config.blockedApps[j])) {
                        monitoredApps[monitoredAppCount].isBlocked = 1;
                        break;
                    }
                }
                
                monitoredAppCount++;
            }
        }
    }
    
    CloseHandle(hProcess);
    return TRUE;
}

void checkRunningApps() {
    if(!config.enableAppMonitoring) return;
    
    time_t now = time(NULL);
    if(now - lastAppCheck < config.appCheckInterval) return;
    
    monitoredAppCount = 0;
    EnumWindows(EnumWindowsProc, 0);
    
    // Check for distracting apps and alert
    for(int i = 0; i < monitoredAppCount; i++) {
        if(monitoredApps[i].isBlocked) {
            char alertMsg[512];
            sprintf(alertMsg, "🚨 DISTRACTION DETECTED! 🚨\n\n"
                    "App: %s\n"
                    "This app is on your blocked list!\n\n"
                    "💪 Stay focused on: %s\n"
                    "⏰ Time since distraction: %.0f seconds",
                    monitoredApps[i].processName, config.subject,
                    difftime(now, monitoredApps[i].startTime));
            
            if(config.enableHardcoreMode) {
                // In hardcore mode, force close the app
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, monitoredApps[i].processId);
                if(hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    strcat(alertMsg, "\n\n🔥 HARDCORE MODE: APP TERMINATED! 🔥");
                }
            }
            
            MessageBox(NULL, alertMsg, "FOCUS DEFENDER", 
                      MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
            
            session.distractionMinutes += (int)difftime(now, monitoredApps[i].startTime) / 60;
        }
    }
    
    lastAppCheck = now;
}

// ENHANCED PARTICLE SYSTEM
void initParticles() {
    for(int i = 0; i < 500; i++) {
        particles[i].life = 0;
    }
}

void addParticle(float x, float y, int type) {
    for(int i = 0; i < 500; i++) {
        if(particles[i].life <= 0) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = (rand() % 200 - 100) / 100.0f;
            particles[i].vy = (rand() % 200 - 100) / 100.0f;
            particles[i].life = particles[i].maxLife = 2.0f + (rand() % 100) / 50.0f;
            particles[i].size = 2.0f + (rand() % 5);
            particles[i].type = type;
            
            switch(type) {
                case 0: // Fire spark
                    particles[i].r = 1.0f;
                    particles[i].g = 0.5f + (rand() % 50) / 100.0f;
                    particles[i].b = 0.0f;
                    break;
                case 1: // Electric glow
                    particles[i].r = 0.3f + (rand() % 70) / 100.0f;
                    particles[i].g = 0.3f + (rand() % 70) / 100.0f;
                    particles[i].b = 1.0f;
                    break;
                case 2: // Success stars
                    particles[i].r = 1.0f;
                    particles[i].g = 1.0f;
                    particles[i].b = 0.3f;
                    break;
            }
            particles[i].a = 1.0f;
            break;
        }
    }
}

void updateParticles(float deltaTime) {
    for(int i = 0; i < 500; i++) {
        if(particles[i].life > 0) {
            particles[i].x += particles[i].vx * deltaTime;
            particles[i].y += particles[i].vy * deltaTime;
            particles[i].life -= deltaTime;
            particles[i].a = particles[i].life / particles[i].maxLife;
            
            // Gravity for fire particles
            if(particles[i].type == 0) {
                particles[i].vy += 50.0f * deltaTime;
            }
        }
    }
}

void renderParticles() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for(int i = 0; i < 500; i++) {
        if(particles[i].life > 0) {
            glColor4f(particles[i].r, particles[i].g, particles[i].b, particles[i].a);
            glPointSize(particles[i].size);
            
            glBegin(GL_POINTS);
            glVertex2f(particles[i].x, particles[i].y);
            glEnd();
        }
    }
    
    glDisable(GL_BLEND);
}

// ENHANCED OPENGL RENDERING
void renderOpenGL() {
    if(!config.enableOpenGL) return;
    
    static float time = 0.0f;
    time += 0.016f; // ~60 FPS
    
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    // Setup viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);
    glMatrixMode(GL_MODELVIEW);
    
    // Animated cosmic background
    glBegin(GL_QUADS);
        glColor3f(0.05f, 0.05f, 0.15f + sin(time) * 0.05f);
        glVertex2f(0, 0);
        glVertex2f(1000, 0);
        glColor3f(0.1f + sin(time * 0.5f) * 0.05f, 0.0f, 0.2f + cos(time) * 0.05f);
        glVertex2f(1000, 700);
        glVertex2f(0, 700);
    glEnd();
    
    // Energy rings
    glColor3f(0.8f + sin(time * 2) * 0.2f, 0.4f, 1.0f);
    for(int ring = 0; ring < 8; ring++) {
        float radius = 50 + ring * 40 + sin(time + ring) * 20;
        glBegin(GL_LINE_LOOP);
        for(int i = 0; i < 36; i++) {
            float angle = i * 10.0f * 3.14159f / 180.0f;
            float x = 500 + radius * cos(angle + time);
            float y = 350 + radius * sin(angle + time);
            glVertex2f(x, y);
        }
        glEnd();
    }
    
    // Neural network visualization
    glColor3f(0.2f + sin(time) * 0.3f, 0.8f, 0.6f);
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for(int i = 0; i < 50; i++) {
        float x = 100 + (i % 10) * 80 + sin(time + i) * 20;
        float y = 100 + (i / 10) * 60 + cos(time * 1.5f + i) * 15;
        glVertex2f(x, y);
        
        // Connect to nearby neurons
        for(int j = i + 1; j < 50; j++) {
            float x2 = 100 + (j % 10) * 80 + sin(time + j) * 20;
            float y2 = 100 + (j / 10) * 60 + cos(time * 1.5f + j) * 15;
            float dist = sqrt((x2-x) * (x2-x) + (y2-y) * (y2-y));
            
            if(dist < 100) {
                glColor4f(0.2f, 0.8f, 0.6f, 1.0f - dist/100.0f);
                glBegin(GL_LINES);
                glVertex2f(x, y);
                glVertex2f(x2, y2);
                glEnd();
            }
        }
    }
    glEnd();
    
    // Floating text particles
    if(rand() % 60 == 0) { // Add text particle occasionally
        addParticle(rand() % 1000, rand() % 700, 2);
    }
    
    updateParticles(0.016f);
    renderParticles();
    
    // Power level indicator
    glColor3f(1.0f, 0.8f, 0.0f);
    char powerText[64];
    sprintf(powerText, "POWER LEVEL: %.1f%%", session.productivityScore);
    
    SwapBuffers(hdc);
    glowIntensity = sin(time * 3) * 0.5f + 0.5f;
}

// ENHANCED SCREEN FLASH
void flashScreen() {
    if(!config.enableFlash) return;
    
    HDC hdc = GetDC(NULL);
    RECT rect;
    GetWindowRect(GetDesktopWindow(), &rect);
    
    // Multi-color flash sequence
    COLORREF colors[] = {
        RGB(255, 100, 0),   // Orange
        RGB(255, 0, 100),   // Pink
        RGB(100, 255, 0),   // Green
        RGB(0, 100, 255),   // Blue
        RGB(255, 255, 0)    // Yellow
    };
    
    for(int flash = 0; flash < 3; flash++) {
        for(int i = 0; i < 5; i++) {
            HBRUSH brush = CreateSolidBrush(colors[i]);
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
            Sleep(30);
        }
        InvalidateRect(NULL, NULL, TRUE);
        Sleep(50);
    }
    ReleaseDC(NULL, hdc);
}

// ENHANCED CONSOLE SHAKE
void shakeConsole() {
    if(!config.enableShake) return;
    
    HWND consoleWindow = GetConsoleWindow();
    RECT rect;
    GetWindowRect(consoleWindow, &rect);
    int originalX = rect.left;
    int originalY = rect.top;
    
    // Epic shake sequence
    for(int i = 0; i < 25; i++) {
        int intensity = config.shakeIntensity;
        SetWindowPos(consoleWindow, HWND_TOPMOST, 
                    originalX + (rand() % (intensity * 2) - intensity), 
                    originalY + (rand() % (intensity * 2) - intensity), 
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
        Sleep(30);
    }
    
    SetWindowPos(consoleWindow, HWND_NOTOPMOST, originalX, originalY, 0, 0, SWP_NOSIZE);
}

// OPENGL WINDOW SETUP AND DISPLAY
LRESULT CALLBACK GLWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_DESTROY:
            return 0;
        case WM_SIZE:
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_KEYDOWN:
            if(wParam == VK_ESCAPE) {
                ShowWindow(hwnd, SW_HIDE);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void initOpenGL() {
    if(!config.enableOpenGL) return;
    
    // Register GL window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = GLWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "StudyReminderGL";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_OWNDC;
    RegisterClass(&wc);
    
    // Create GL window
    hwndGL = CreateWindow("StudyReminderGL", "🔥 STUDY MODE VISUALIZATION 🔥", 
                         WS_OVERLAPPEDWINDOW, 100, 100, 1000, 700, 
                         NULL, NULL, GetModuleHandle(NULL), NULL);
    
    hdc = GetDC(hwndGL);
    
    // Setup pixel format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    
    int format = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, format, &pfd);
    hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hrc);
    
    // GL settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
}

void showGLWindow() {
    if(!config.enableOpenGL || !hwndGL) return;
    
    ShowWindow(hwndGL, SW_SHOW);
    UpdateWindow(hwndGL);
    SetWindowPos(hwndGL, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    // Show for 10 seconds with epic animation
    for(int i = 0; i < 600; i++) { // 10 seconds at ~60fps
        renderOpenGL();
        
        // Add explosion particles periodically
        if(i % 30 == 0) {
            for(int j = 0; j < 10; j++) {
                addParticle(500 + rand() % 200 - 100, 350 + rand() % 200 - 100, rand() % 3);
            }
        }
        
        Sleep(16); // ~60fps
        
        MSG msg;
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    ShowWindow(hwndGL, SW_HIDE);
}

// DARK MODE GUI FUNCTIONS
HBRUSH CreateDarkBrush(COLORREF color) {
    return CreateSolidBrush(color);
}

LRESULT CALLBACK DarkEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch(msg) {
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(45, 45, 45));
            return (LRESULT)CreateSolidBrush(RGB(45, 45, 45));
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// MAIN WINDOW PROCEDURE
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_CREATE: {
            // Create dark mode fonts
            mainFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            boldFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            
            // Create controls
            CreateWindow("STATIC", "🔥 SUBJECT:", WS_VISIBLE | WS_CHILD,
                        20, 20, 120, 25, hwnd, NULL, NULL, NULL);
            
            hSubjectEdit = CreateWindow("EDIT", config.subject, WS_VISIBLE | WS_CHILD | WS_BORDER,
                                      150, 20, 200, 25, hwnd, (HMENU)IDC_SUBJECT_EDIT, NULL, NULL);
            
            CreateWindow("STATIC", "⏱️ INTERVAL (min):", WS_VISIBLE | WS_CHILD,
                        370, 20, 120, 25, hwnd, NULL, NULL, NULL);
            
            hIntervalEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                       500, 20, 80, 25, hwnd, (HMENU)IDC_INTERVAL_EDIT, NULL, NULL);
            
            char intervalStr[16];
            sprintf(intervalStr, "%d", config.reminderInterval);
            SetWindowText(hIntervalEdit, intervalStr);
            
            // Checkboxes with emojis
            CreateWindow("BUTTON", "💥 Flash Screen", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        20, 60, 150, 25, hwnd, (HMENU)IDC_FLASH_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "🎵 Sound Alert", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        180, 60, 150, 25, hwnd, (HMENU)IDC_SOUND_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "📱 Shake Window", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        340, 60, 150, 25, hwnd, (HMENU)IDC_SHAKE_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "🌟 OpenGL Effects", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        500, 60, 150, 25, hwnd, (HMENU)IDC_OPENGL_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "👀 App Monitoring", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        20, 90, 150, 25, hwnd, (HMENU)IDC_APP_MONITOR_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "🎯 Focus Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        180, 90, 150, 25, hwnd, (HMENU)IDC_FOCUS_MODE_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "💀 Hardcore Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        340, 90, 150, 25, hwnd, (HMENU)IDC_HARDCORE_CHECK, NULL, NULL);
            
            CreateWindow("BUTTON", "🌙 Dark Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                        500, 90, 150, 25, hwnd, (HMENU)IDC_DARK_MODE_CHECK, NULL, NULL);
            
            // Control buttons
            CreateWindow("BUTTON", "🚀 START DOMINATION", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        20, 130, 200, 40, hwnd, (HMENU)IDC_START_BUTTON, NULL, NULL);
            
            CreateWindow("BUTTON", "⏹️ STOP SESSION", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        240, 130, 150, 40, hwnd, (HMENU)IDC_STOP_BUTTON, NULL, NULL);
            
            CreateWindow("BUTTON", "📊 VIEW STATS", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        410, 130, 120, 40, hwnd, (HMENU)IDC_STATS_BUTTON, NULL, NULL);
            
            CreateWindow("BUTTON", "⚙️ CONFIG", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        550, 130, 100, 40, hwnd, (HMENU)IDC_CONFIG_BUTTON, NULL, NULL);
            
            // Session info display
            CreateWindow("STATIC", "📈 SESSION ANALYTICS", WS_VISIBLE | WS_CHILD,
                        20, 190, 200, 25, hwnd, NULL, NULL, NULL);
            
            CreateWindow("STATIC", "Study Time:", WS_VISIBLE | WS_CHILD,
                        20, 220, 100, 20, hwnd, NULL, NULL, NULL);
            
            hSessionTime = CreateWindow("STATIC", "0 min", WS_VISIBLE | WS_CHILD,
                                      130, 220, 80, 20, hwnd, (HMENU)IDC_SESSION_TIME, NULL, NULL);
            
            CreateWindow("STATIC", "Distraction:", WS_VISIBLE | WS_CHILD,
                        220, 220, 100, 20, hwnd, NULL, NULL, NULL);
            
            hDistractionTime = CreateWindow("STATIC", "0 min", WS_VISIBLE | WS_CHILD,
                                          330, 220, 80, 20, hwnd, (HMENU)IDC_DISTRACTION_TIME, NULL, NULL);
            
            CreateWindow("STATIC", "🔥 Streak:", WS_VISIBLE | WS_CHILD,
                        420, 220, 60, 20, hwnd, NULL, NULL, NULL);
            
            hStreakCounter = CreateWindow("STATIC", "0 days", WS_VISIBLE | WS_CHILD,
                                        490, 220, 80, 20, hwnd, (HMENU)IDC_STREAK_COUNTER, NULL, NULL);
            
            // Productivity bar
            CreateWindow("STATIC", "💪 PRODUCTIVITY SCORE", WS_VISIBLE | WS_CHILD,
                        20, 250, 200, 20, hwnd, NULL, NULL, NULL);
            
            hProductivityBar = CreateWindow("PROGRESS_CLASS", "", WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
                                          20, 275, 400, 25, hwnd, (HMENU)IDC_PRODUCTIVITY_BAR, NULL, NULL);
            
            SendMessage(hProductivityBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            
            // Set checkbox states
            CheckDlgButton(hwnd, IDC_FLASH_CHECK, config.enableFlash);
            CheckDlgButton(hwnd, IDC_SOUND_CHECK, config.enableSound);
            CheckDlgButton(hwnd, IDC_SHAKE_CHECK, config.enableShake);
            CheckDlgButton(hwnd, IDC_OPENGL_CHECK, config.enableOpenGL);
            CheckDlgButton(hwnd, IDC_APP_MONITOR_CHECK, config.enableAppMonitoring);
            CheckDlgButton(hwnd, IDC_FOCUS_MODE_CHECK, config.enableFocusMode);
            CheckDlgButton(hwnd, IDC_HARDCORE_CHECK, config.enableHardcoreMode);
            CheckDlgButton(hwnd, IDC_DARK_MODE_CHECK, config.enableDarkMode);
            
            // Initialize session display
            updateSessionDisplay();
            
            break;
        }
        
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            if(config.enableDarkMode) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(220, 220, 220));
                SetBkColor(hdc, RGB(45, 45, 45));
                return (LRESULT)CreateSolidBrush(RGB(45, 45, 45));
            }
            break;
        }
        
        case WM_CTLCOLORBTN: {
            if(config.enableDarkMode) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(60, 60, 60));
                return (LRESULT)CreateSolidBrush(RGB(60, 60, 60));
            }
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if(config.enableDarkMode) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                FillRect(hdc, &rect, CreateSolidBrush(RGB(32, 32, 32)));
            }
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case IDC_START_BUTTON:
                    startStudySession();
                    break;
                    
                case IDC_STOP_BUTTON:
                    stopStudySession();
                    break;
                    
                case IDC_STATS_BUTTON:
                    showDetailedStats();
                    break;
                    
                case IDC_CONFIG_BUTTON:
                    showConfigWindow();
                    break;
                    
                case IDC_FLASH_CHECK:
                    config.enableFlash = IsDlgButtonChecked(hwnd, IDC_FLASH_CHECK);
                    break;
                    
                case IDC_SOUND_CHECK:
                    config.enableSound = IsDlgButtonChecked(hwnd, IDC_SOUND_CHECK);
                    break;
                    
                case IDC_SHAKE_CHECK:
                    config.enableShake = IsDlgButtonChecked(hwnd, IDC_SHAKE_CHECK);
                    break;
                    
                case IDC_OPENGL_CHECK:
                    config.enableOpenGL = IsDlgButtonChecked(hwnd, IDC_OPENGL_CHECK);
                    break;
                    
                case IDC_APP_MONITOR_CHECK:
                    config.enableAppMonitoring = IsDlgButtonChecked(hwnd, IDC_APP_MONITOR_CHECK);
                    break;
                    
                case IDC_FOCUS_MODE_CHECK:
                    config.enableFocusMode = IsDlgButtonChecked(hwnd, IDC_FOCUS_MODE_CHECK);
                    break;
                    
                case IDC_HARDCORE_CHECK:
                    config.enableHardcoreMode = IsDlgButtonChecked(hwnd, IDC_HARDCORE_CHECK);
                    if(config.enableHardcoreMode) {
                        MessageBox(hwnd, "⚠️ HARDCORE MODE ACTIVATED! ⚠️\n\n"
                                  "Blocked apps will be FORCE CLOSED!\n"
                                  "Prepare for MAXIMUM FOCUS!", 
                                  "HARDCORE ENGAGED", MB_OK | MB_ICONWARNING);
                    }
                    break;
                    
                case IDC_DARK_MODE_CHECK:
                    config.enableDarkMode = IsDlgButtonChecked(hwnd, IDC_DARK_MODE_CHECK);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
            }
            break;
        }
        
        case WM_TIMER: {
            switch(wParam) {
                case 1: // Main reminder timer
                    launchUniversalNotification();
                    break;
                    
                case 2: // App monitoring timer
                    checkRunningApps();
                    updateSessionDisplay();
                    calculateProductivityScore();
                    break;
                    
                case 3: // OpenGL animation timer
                    if(hwndGL && config.enableOpenGL) {
                        renderOpenGL();
                    }
                    break;
            }
            break;
        }
        
        case WM_DESTROY:
            saveSessionData();
            saveConfig();
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE:
            if(hwndGL) {
                RECT rect;
                GetClientRect(hwndGL, &rect);
                glViewport(0, 0, rect.right, rect.bottom);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// SESSION MANAGEMENT FUNCTIONS
void startStudySession() {
    if(isRunning) return;
    
    // Get updated config from GUI
    char buffer[128];
    GetWindowText(hSubjectEdit, config.subject, sizeof(config.subject));
    GetWindowText(hIntervalEdit, buffer, sizeof(buffer));
    config.reminderInterval = atoi(buffer);
    
    sessionStartTime = time(NULL);
    session.sessionStart = sessionStartTime;
    isRunning = 1;
    
    // Start timers
    SetTimer(hwndMain, 1, config.reminderInterval * 60 * 1000, NULL); // Main reminder
    SetTimer(hwndMain, 2, config.appCheckInterval * 1000, NULL);      // App monitoring
    SetTimer(hwndMain, 3, 16, NULL);                                  // OpenGL animation (~60fps)
    
    // Launch study apps if enabled
    if(config.enableAutoLaunch) {
        launchStudyApps();
    }
    
    // Initialize OpenGL if enabled
    if(config.enableOpenGL) {
        initOpenGL();
        initParticles();
    }
    
    // Epic startup message
    char startMsg[512];
    sprintf(startMsg, "🚀 STUDY DOMINATION ACTIVATED! 🚀\n\n"
                     "Subject: %s\n"
                     "Reminder Interval: %d minutes\n"
                     "App Monitoring: %s\n"
                     "Hardcore Mode: %s\n\n"
                     "💪 TIME TO UNLEASH YOUR POTENTIAL! 💪",
            config.subject, config.reminderInterval,
            config.enableAppMonitoring ? "ENABLED" : "DISABLED",
            config.enableHardcoreMode ? "⚠️ ACTIVE ⚠️" : "DISABLED");
    
    MessageBox(hwndMain, startMsg, "SYSTEM ACTIVATED", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    
    // Update button states
    EnableWindow(GetDlgItem(hwndMain, IDC_START_BUTTON), FALSE);
    EnableWindow(GetDlgItem(hwndMain, IDC_STOP_BUTTON), TRUE);
    SetWindowText(GetDlgItem(hwndMain, IDC_START_BUTTON), "🔥 ACTIVE");
}

void stopStudySession() {
    if(!isRunning) return;
    
    isRunning = 0;
    KillTimer(hwndMain, 1);
    KillTimer(hwndMain, 2);
    KillTimer(hwndMain, 3);
    
    // Calculate session stats
    time_t sessionEnd = time(NULL);
    int sessionMinutes = (int)difftime(sessionEnd, sessionStartTime) / 60;
    session.studyMinutes += sessionMinutes;
    session.totalSessions++;
    
    if(session.distractionMinutes == 0) {
        session.perfectSessions++;
    }
    
    calculateProductivityScore();
    saveSessionData();
    
    // Session complete message
    char endMsg[512];
    sprintf(endMsg, "📊 SESSION COMPLETE! 📊\n\n"
                   "Study Time: %d minutes\n"
                   "Distractions: %d minutes\n"
                   "Productivity Score: %.1f%%\n"
                   "Reminders Triggered: %d\n\n"
                   "🏆 GREAT WORK! KEEP THE MOMENTUM! 🏆",
           sessionMinutes, session.distractionMinutes, 
           session.productivityScore, session.reminderCount);
    
    MessageBox(hwndMain, endMsg, "SESSION STATS", MB_OK | MB_ICONINFORMATION);
    
    // Reset for next session
    session.distractionMinutes = 0;
    session.reminderCount = 0;
    
    // Update button states
    EnableWindow(GetDlgItem(hwndMain, IDC_START_BUTTON), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_STOP_BUTTON), FALSE);
    SetWindowText(GetDlgItem(hwndMain, IDC_START_BUTTON), "🚀 START DOMINATION");
}

void updateSessionDisplay() {
    if(!isRunning) return;
    
    time_t now = time(NULL);
    int currentSessionMinutes = (int)difftime(now, sessionStartTime) / 60;
    
    char timeStr[64];
    sprintf(timeStr, "%d min", currentSessionMinutes);
    SetWindowText(hSessionTime, timeStr);
    
    sprintf(timeStr, "%d min", session.distractionMinutes);
    SetWindowText(hDistractionTime, timeStr);
    
    sprintf(timeStr, "%d days", session.streakDays);
    SetWindowText(hStreakCounter, timeStr);
    
    // Update productivity bar
    SendMessage(hProductivityBar, PBM_SETPOS, (int)session.productivityScore, 0);
}

void calculateProductivityScore() {
    if(session.studyMinutes + session.distractionMinutes == 0) {
        session.productivityScore = 100.0f;
        return;
    }
    
    session.productivityScore = ((float)session.studyMinutes / 
                               (session.studyMinutes + session.distractionMinutes)) * 100.0f;
    
    // Bonus for consistency
    if(session.reminderCount > 0) {
        session.productivityScore += (session.reminderCount * 2.0f);
    }
    
    if(session.productivityScore > 100.0f) session.productivityScore = 100.0f;
    if(session.productivityScore < 0.0f) session.productivityScore = 0.0f;
}

// ENHANCED APP LAUNCHING
void launchStudyApps() {
    for(int i = 0; i < config.studyAppCount; i++) {
        // Try different common paths
        char paths[][256] = {
            "C:\\Program Files\\%s",
            "C:\\Program Files (x86)\\%s",
            "C:\\Windows\\System32\\%s",
            "%s" // Direct path
        };
        
        for(int j = 0; j < 4; j++) {
            char fullPath[512];
            sprintf(fullPath, paths[j], config.studyApps[i]);
            
            HINSTANCE result = ShellExecute(NULL, "open", fullPath, NULL, NULL, SW_SHOWNORMAL);
            if((INT_PTR)result > 32) {
                break; // Success
            }
        }
        Sleep(500);
    }
}

// ACHIEVEMENT SYSTEM
void checkAchievements() {
    static int lastReminderCheck = 0;
    static int lastStreakCheck = 0;
    static int lastScoreCheck = 0;
    
    char achievement[512];
    int newAchievement = 0;
    
    // Reminder milestones
    if(session.reminderCount >= 10 && lastReminderCheck < 10) {
        sprintf(achievement, "🏆 ACHIEVEMENT UNLOCKED! 🏆\n\n"
                           "REMINDER WARRIOR\n"
                           "Completed 10 study reminders!\n\n"
                           "Your dedication is inspiring!");
        newAchievement = 1;
        lastReminderCheck = 10;
    }
    else if(session.reminderCount >= 50 && lastReminderCheck < 50) {
        sprintf(achievement, "🏆 LEGENDARY ACHIEVEMENT! 🏆\n\n"
                           "STUDY MACHINE\n"
                           "50 reminders conquered!\n\n"
                           "You are unstoppable!");
        newAchievement = 1;
        lastReminderCheck = 50;
    }
    
    // Productivity score achievements
    if(session.productivityScore >= 90.0f && lastScoreCheck < 90) {
        sprintf(achievement, "⭐ EXCELLENCE ACHIEVED! ⭐\n\n"
                           "PRODUCTIVITY MASTER\n"
                           "Maintained 90%+ productivity!\n\n"
                           "You're in the zone!");
        newAchievement = 1;
        lastScoreCheck = 90;
    }
    
    // Perfect session achievement
    if(session.distractionMinutes == 0 && session.studyMinutes > 30) {
        sprintf(achievement, "💎 PERFECT SESSION! 💎\n\n"
                           "ZERO DISTRACTIONS\n"
                           "Pure focus achieved!\n\n"
                           "This is mastery!");
        newAchievement = 1;
    }
    
    if(newAchievement) {
        session.achievementCount++;
        MessageBox(hwndMain, achievement, "ACHIEVEMENT UNLOCKED", 
                  MB_OK | MB_ICONASTERISK | MB_TOPMOST);
        
        // Add celebration particles
        if(config.enableOpenGL) {
            for(int i = 0; i < 50; i++) {
                addParticle(rand() % 1000, rand() % 700, 2);
            }
        }
    }
}

// ENHANCED NOTIFICATION SYSTEM
void launchUniversalNotification() {
    session.reminderCount++;
    time_t totalStudyTime = session.studyMinutes + (time(NULL) - sessionStartTime) / 60;
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    // Clear console and show epic header
    system("cls");
    SetConsoleTextAttribute(hConsole, 12); // Bright red
    
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    🔥 STUDY REMINDER ACTIVATED 🔥             ║\n");
    printf("║                         %s                        ║\n", config.subject);
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  🎯 SESSION: #%-8d                                      ║\n", session.reminderCount);
    printf("║  ⏰ TIME: %02d:%02d:%02d                                        ║\n", st.wHour, st.wMinute, st.wSecond);
    printf("║  📊 PRODUCTIVITY: %.1f%%                                      ║\n", session.productivityScore);
    printf("║  🏆 ACHIEVEMENTS: %-8d                                 ║\n", session.achievementCount);
    printf("║  🔥 STREAK: %d days                                           ║\n", session.streakDays);
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    // Effects sequence
    if(config.enableShake) shakeConsole();
    if(config.enableSound) playEnhancedAlarmSounds();
    if(config.enableFlash) flashScreen();
    if(config.enableOpenGL) showGLWindow();
    
    // Show motivational message
    if(config.messageCount > 0) {
        const char* message = config.messages[rand() % config.messageCount];
        
        char fullMessage[1024];
        sprintf(fullMessage, "%s\n\n"
                           "🎯 FOCUS TARGET: %s\n"
                           "📈 SESSION #%d\n"
                           "💪 PRODUCTIVITY: %.1f%%\n"
                           "🏆 ACHIEVEMENTS: %d\n\n"
                           "⚡ TIME TO DOMINATE! ⚡", 
                message, config.subject, session.reminderCount, 
                session.productivityScore, session.achievementCount);
        
        MessageBox(NULL, fullMessage, "🔥 STUDY PROTOCOL ACTIVATED 🔥", 
                   MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST | MB_SETFOREGROUND);
    }
    
    // Check for achievements
    checkAchievements();
    
    // Update display
    updateSessionDisplay();
}

// ENHANCED SOUND SYSTEM
void playEnhancedAlarmSounds() {
    if(!config.enableSound) return;
    
    // Epic sound sequence
    for(int i = 0; i < 5; i++) {
        Beep(800 + i * 100, 100);
        Beep(1200 - i * 50, 100);
    }
    
    // System sounds
    PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
    Sleep(200);
    PlaySound(TEXT("SystemAsterisk"), NULL, SND_ALIAS | SND_ASYNC);
}

// DETAILED STATS WINDOW
void showDetailedStats() {
    char stats[2048];
    time_t now = time(NULL);
    int currentSessionTime = isRunning ? (int)difftime(now, sessionStartTime) / 60 : 0;
    
    sprintf(stats, "📊 DETAILED STUDY ANALYTICS 📊\n\n"
                  "🎯 CURRENT SESSION:\n"
                  "• Subject: %s\n"
                  "• Active Time: %d minutes\n"
                  "• Distractions: %d minutes\n"
                  "• Reminders: %d\n"
                  "• Status: %s\n\n"
                  "📈 OVERALL STATISTICS:\n"
                  "• Total Study Time: %d minutes\n"
                  "• Total Sessions: %d\n"
                  "• Perfect Sessions: %d\n"
                  "• Achievement Count: %d\n"
                  "• Current Streak: %d days\n"
                  "• Avg Productivity: %.1f%%\n\n"
                  "🏆 PERFORMANCE METRICS:\n"
                  "• Focus Score: %.1f%%\n"
                  "• Consistency Rating: %s\n"
                  "• Study Warrior Level: %s\n\n"
                  "Keep pushing forward! 💪",
          config.subject, currentSessionTime, session.distractionMinutes,
          session.reminderCount, isRunning ? "🔥 ACTIVE" : "⏸️ PAUSED",
          session.studyMinutes, session.totalSessions, session.perfectSessions,
          session.achievementCount, session.streakDays, session.productivityScore,
          session.productivityScore,
          session.productivityScore > 80 ? "EXCELLENT" : session.productivityScore > 60 ? "GOOD" : "NEEDS FOCUS",
          session.reminderCount > 50 ? "LEGENDARY" : session.reminderCount > 20 ? "ADVANCED" : session.reminderCount > 5 ? "INTERMEDIATE" : "BEGINNER");
    
    MessageBox(hwndMain, stats, "📊 STUDY ANALYTICS DASHBOARD", MB_OK | MB_ICONINFORMATION);
}

// CONFIG WINDOW (Placeholder for now - full implementation would be extensive)
void showConfigWindow() {
    MessageBox(hwndMain, 
              "⚙️ ADVANCED CONFIGURATION ⚙️\n\n"
              "Edit 'study_reminder.conf' to customize:\n"
              "• Custom messages and quotes\n"
              "• Study app paths\n"
              "• Blocked app list\n"
              "• Sound and visual settings\n"
              "• Achievement thresholds\n\n"
              "Restart the application after editing config file.",
              "CONFIG MANAGER", MB_OK | MB_ICONINFORMATION);
}

// CONFIG SAVE FUNCTION
void saveConfig() {
    FILE* file = fopen("study_reminder.conf", "w");
    if(!file) return;
    
    fprintf(file, "# ENHANCED UNIVERSAL STUDY REMINDER CONFIG v2.0\n");
    fprintf(file, "SUBJECT=%s\n", config.subject);
    fprintf(file, "REMINDER_INTERVAL=%d\n", config.reminderInterval);
    fprintf(file, "ENABLE_FLASH=%d\n", config.enableFlash);
    fprintf(file, "ENABLE_SHAKE=%d\n", config.enableShake);
    fprintf(file, "ENABLE_SOUND=%d\n", config.enableSound);
    fprintf(file, "ENABLE_OPENGL=%d\n", config.enableOpenGL);
    fprintf(file, "ENABLE_APP_MONITORING=%d\n", config.enableAppMonitoring);
    fprintf(file, "ENABLE_FOCUS_MODE=%d\n", config.enableFocusMode);
    fprintf(file, "ENABLE_HARDCORE_MODE=%d\n", config.enableHardcoreMode);
    fprintf(file, "ENABLE_DARK_MODE=%d\n", config.enableDarkMode);
    // ... save other config values
    
    fclose(file);
}

// MAIN FUNCTION
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    srand((unsigned int)time(NULL));
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    loadConfig();
    loadSessionData();
    
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Register window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "StudyReminderGUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = config.enableDarkMode ? CreateSolidBrush(RGB(32, 32, 32)) : (HBRUSH)(COLOR_WINDOW+1);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClass(&wc);
    
    // Create main window
    hwndMain = CreateWindow("StudyReminderGUI", 
                           "🔥 UNIVERSAL STUDY DOMINATION SYSTEM v2.0 🔥", 
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
                           700, 350, NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
    
    // Epic startup sequence
    MessageBox(hwndMain, 
              "🚀 STUDY DOMINATION SYSTEM LOADED! 🚀\n\n"
              "Features Activated:\n"
              "✅ Dark Mode GUI\n"
              "✅ Real-time App Monitoring\n"
              "✅ Enhanced OpenGL Effects\n"
              "✅ Achievement System\n"
              "✅ Productivity Analytics\n"
              "✅ Hardcore Focus Mode\n"
              "✅ Session Tracking\n\n"
              "Ready to unleash your potential! 💪",
              "SYSTEM INITIALIZED", MB_OK | MB_ICONINFORMATION);
    
    // Message loop
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
