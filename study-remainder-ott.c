#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <GL/glu.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")

// CONFIG STRUCTURES
typedef struct {
    char messages[50][256];
    int messageCount;
    char appPaths[20][512];
    int appCount;
    int reminderInterval;
    int enableFlash;
    int enableShake;
    int enableSound;
    int enableOpenGL;
    char subject[64];
} Config;

// GLOBAL VARIABLES
Config config;
int reminderCount = 0;
int totalStudyTime = 0;
HWND hwnd, consoleWindow;
HANDLE hConsole;
HDC hdc;
HGLRC hrc;
float glowIntensity = 0.0f;
float particleSystem[100][4]; // x, y, vx, vy for 100 particles

// DEFAULT CONFIG CREATION
void createDefaultConfig() {
    FILE* file = fopen("study_reminder.conf", "w");
    if(!file) return;
    
    fprintf(file, "# UNIVERSAL STUDY REMINDER CONFIG\n");
    fprintf(file, "# Edit this file to customize your experience\n\n");
    
    fprintf(file, "SUBJECT=Universal Learning\n");
    fprintf(file, "REMINDER_INTERVAL=25\n");
    fprintf(file, "ENABLE_FLASH=1\n");
    fprintf(file, "ENABLE_SHAKE=1\n");
    fprintf(file, "ENABLE_SOUND=1\n");
    fprintf(file, "ENABLE_OPENGL=1\n\n");
    
    fprintf(file, "# MOTIVATIONAL MESSAGES (one per line after MESSAGES:)\n");
    fprintf(file, "MESSAGES:\n");
    fprintf(file, "TIME TO DOMINATE YOUR STUDIES!\n");
    fprintf(file, "KNOWLEDGE AWAITS - GO LEARN SOMETHING EPIC!\n");
    fprintf(file, "BRAIN UPGRADE IN PROGRESS - STUDY NOW!\n");
    fprintf(file, "YOUR FUTURE SELF IS CHEERING YOU ON!\n");
    fprintf(file, "ACADEMIC BEAST MODE ACTIVATED!\n");
    fprintf(file, "LEARNING IS YOUR SUPERPOWER - USE IT!\n");
    fprintf(file, "CRUSH THOSE CONCEPTS LIKE A CHAMPION!\n");
    fprintf(file, "STUDY TIME = GROWTH TIME!\n");
    fprintf(file, "UNLOCK YOUR POTENTIAL - HIT THE BOOKS!\n");
    fprintf(file, "MASTER MODE ENGAGED - STUDY PROTOCOL ACTIVE!\n");
    fprintf(file, "YOUR BRAIN DEMANDS FRESH KNOWLEDGE!\n");
    fprintf(file, "EXCELLENCE IS CALLING - ANSWER WITH STUDY!\n");
    fprintf(file, "TRANSFORM YOUR MIND - STUDY SESSION NOW!\n");
    fprintf(file, "LEVEL UP YOUR INTELLIGENCE - GO STUDY!\n");
    fprintf(file, "END_MESSAGES\n\n");
    
    fprintf(file, "# APP PATHS (launches when study mode activates)\n");
    fprintf(file, "APPS:\n");
    fprintf(file, "C:\\Program Files\\Notepad++\\notepad++.exe\n");
    fprintf(file, "C:\\Windows\\System32\\calc.exe\n");
    fprintf(file, "END_APPS\n");
    
    fclose(file);
}

// CONFIG LOADER
void loadConfig() {
    FILE* file = fopen("study_reminder.conf", "r");
    if(!file) {
        createDefaultConfig();
        file = fopen("study_reminder.conf", "r");
    }
    
    // Set defaults
    strcpy(config.subject, "Universal Learning");
    config.reminderInterval = 25;
    config.enableFlash = 1;
    config.enableShake = 1;
    config.enableSound = 1;
    config.enableOpenGL = 1;
    config.messageCount = 0;
    config.appCount = 0;
    
    char line[512];
    int inMessages = 0, inApps = 0;
    
    while(fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        if(line[0] == '#' || strlen(line) == 0) continue;
        
        if(strncmp(line, "SUBJECT=", 8) == 0) {
            strcpy(config.subject, line + 8);
        }
        else if(strncmp(line, "REMINDER_INTERVAL=", 18) == 0) {
            config.reminderInterval = atoi(line + 18);
        }
        else if(strncmp(line, "ENABLE_FLASH=", 13) == 0) {
            config.enableFlash = atoi(line + 13);
        }
        else if(strncmp(line, "ENABLE_SHAKE=", 13) == 0) {
            config.enableShake = atoi(line + 13);
        }
        else if(strncmp(line, "ENABLE_SOUND=", 13) == 0) {
            config.enableSound = atoi(line + 13);
        }
        else if(strncmp(line, "ENABLE_OPENGL=", 14) == 0) {
            config.enableOpenGL = atoi(line + 14);
        }
        else if(strcmp(line, "MESSAGES:") == 0) {
            inMessages = 1;
            inApps = 0;
        }
        else if(strcmp(line, "END_MESSAGES") == 0) {
            inMessages = 0;
        }
        else if(strcmp(line, "APPS:") == 0) {
            inApps = 1;
            inMessages = 0;
        }
        else if(strcmp(line, "END_APPS") == 0) {
            inApps = 0;
        }
        else if(inMessages && config.messageCount < 50) {
            strcpy(config.messages[config.messageCount], line);
            config.messageCount++;
        }
        else if(inApps && config.appCount < 20) {
            strcpy(config.appPaths[config.appCount], line);
            config.appCount++;
        }
    }
    
    fclose(file);
}

// OPENGL SETUP
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void initOpenGL() {
    if(!config.enableOpenGL) return;
    
    // Register window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "StudyReminderGL";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);
    
    // Create window
    hwnd = CreateWindow("StudyReminderGL", "STUDY MODE ACTIVATED", 
                       WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
                       800, 600, NULL, NULL, GetModuleHandle(NULL), NULL);
    
    hdc = GetDC(hwnd);
    
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
    
    // Initialize particle system
    for(int i = 0; i < 100; i++) {
        particleSystem[i][0] = (rand() % 800);
        particleSystem[i][1] = (rand() % 600);
        particleSystem[i][2] = (rand() % 10 - 5) * 0.1f;
        particleSystem[i][3] = (rand() % 10 - 5) * 0.1f;
    }
}

void renderOpenGL() {
    if(!config.enableOpenGL) return;
    
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    // Setup viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    
    // Animated background gradient
    glBegin(GL_QUADS);
        glColor3f(0.1f + glowIntensity * 0.3f, 0.0f, 0.3f + glowIntensity * 0.2f);
        glVertex2f(0, 0);
        glVertex2f(800, 0);
        glColor3f(0.0f, 0.1f + glowIntensity * 0.2f, 0.5f + glowIntensity * 0.3f);
        glVertex2f(800, 600);
        glVertex2f(0, 600);
    glEnd();
    
    // Pulsing circles
    glColor3f(1.0f, 0.5f + glowIntensity * 0.5f, 0.0f);
    for(int i = 0; i < 5; i++) {
        glBegin(GL_LINE_LOOP);
        for(int j = 0; j < 360; j += 10) {
            float x = 400 + (50 + i * 30 + glowIntensity * 20) * cos(j * 3.14159f / 180.0f);
            float y = 300 + (50 + i * 30 + glowIntensity * 20) * sin(j * 3.14159f / 180.0f);
            glVertex2f(x, y);
        }
        glEnd();
    }
    
    // Particle system
    glColor3f(1.0f, 1.0f, 0.5f + glowIntensity);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for(int i = 0; i < 100; i++) {
        glVertex2f(particleSystem[i][0], particleSystem[i][1]);
        
        // Update particle
        particleSystem[i][0] += particleSystem[i][2];
        particleSystem[i][1] += particleSystem[i][3];
        
        // Wrap around screen
        if(particleSystem[i][0] < 0) particleSystem[i][0] = 800;
        if(particleSystem[i][0] > 800) particleSystem[i][0] = 0;
        if(particleSystem[i][1] < 0) particleSystem[i][1] = 600;
        if(particleSystem[i][1] > 600) particleSystem[i][1] = 0;
    }
    glEnd();
    
    // Text rendering (simplified)
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(300, 300);
    
    SwapBuffers(hdc);
    
    glowIntensity += 0.05f;
    if(glowIntensity > 1.0f) glowIntensity = 0.0f;
}

void showGLWindow() {
    if(!config.enableOpenGL) return;
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    // Show for 5 seconds with animation
    for(int i = 0; i < 100; i++) {
        renderOpenGL();
        Sleep(50);
        
        MSG msg;
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    ShowWindow(hwnd, SW_HIDE);
}

// APP LAUNCHER
void launchStudyApps() {
    for(int i = 0; i < config.appCount; i++) {
        ShellExecute(NULL, "open", config.appPaths[i], NULL, NULL, SW_SHOWNORMAL);
        Sleep(500); // Delay between launches
    }
}

// CONSOLE EFFECTS
void setRandomColor() {
    int colors[] = {10, 11, 12, 13, 14, 15, 9};
    SetConsoleTextAttribute(hConsole, colors[rand() % 7]);
}

void shakeConsole() {
    if(!config.enableShake) return;
    
    RECT rect;
    GetWindowRect(consoleWindow, &rect);
    int originalX = rect.left;
    int originalY = rect.top;
    
    for(int i = 0; i < 15; i++) {
        SetWindowPos(consoleWindow, HWND_TOPMOST, 
                    originalX + (rand() % 15 - 7), 
                    originalY + (rand() % 15 - 7), 
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
        Sleep(40);
    }
    
    SetWindowPos(consoleWindow, HWND_TOPMOST, originalX, originalY, 0, 0, SWP_NOSIZE);
}

void playAlarmSounds() {
    if(!config.enableSound) return;
    
    for(int i = 0; i < 3; i++) {
        Beep(1000 + i * 200, 150);
        Beep(800 - i * 100, 150);
    }
    
    PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
}

void flashScreen() {
    if(!config.enableFlash) return;
    
    HDC hdc = GetDC(NULL);
    RECT rect;
    GetWindowRect(GetDesktopWindow(), &rect);
    
    for(int i = 0; i < 2; i++) {
        HBRUSH brush = CreateSolidBrush(RGB(255, 100, 0));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
        Sleep(80);
        InvalidateRect(NULL, NULL, TRUE);
        Sleep(80);
    }
    ReleaseDC(NULL, hdc);
}

// MAIN NOTIFICATION SEQUENCE
void launchUniversalNotification() {
    reminderCount++;
    totalStudyTime += config.reminderInterval;
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    system("cls");
    
    // Header
    setRandomColor();
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║              UNIVERSAL STUDY REMINDER                      ║\n");
    printf("║                    %s                     ║\n", config.subject);
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  SESSION: #%d                                             ║\n", reminderCount);
    printf("║  TIME: %02d:%02d:%02d                                      ║\n", st.wHour, st.wMinute, st.wSecond);
    printf("║  TOTAL TRACKED: %d minutes                                ║\n", totalStudyTime);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Launch apps first
    if(config.appCount > 0) {
        printf("Launching study applications...\n");
        launchStudyApps();
    }
    
    // Effects sequence
    shakeConsole();
    playAlarmSounds();
    flashScreen();
    showGLWindow();
    
    // Show random message
    if(config.messageCount > 0) {
        const char* message = config.messages[rand() % config.messageCount];
        
        char fullMessage[512];
        sprintf(fullMessage, "%s\n\nSession #%d - %s Study Time!\nTotal Progress: %d minutes", 
                message, reminderCount, config.subject, totalStudyTime);
        
        MessageBox(NULL, fullMessage, "STUDY PROTOCOL ACTIVATED", 
                   MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST | MB_SETFOREGROUND);
    }
    
    // Achievement system
    if(reminderCount % 5 == 0) {
        char achievement[256];
        sprintf(achievement, "ACHIEVEMENT UNLOCKED!\n'%s Warrior' - %d Sessions!\n\nYour dedication is incredible!", 
                config.subject, reminderCount);
        MessageBox(NULL, achievement, "ACHIEVEMENT", MB_OK | MB_ICONASTERISK | MB_TOPMOST);
    }
    
    // Stats display
    SetConsoleTextAttribute(hConsole, 15);
    printf("\nSTUDY ANALYTICS:\n");
    printf("• Subject: %s\n", config.subject);
    printf("• Sessions completed: %d\n", reminderCount);
    printf("• Total time tracked: %d minutes\n", totalStudyTime);
    printf("• Apps auto-launched: %d\n", config.appCount);
    printf("• Next reminder: %d minutes\n\n", config.reminderInterval);
    
    setRandomColor();
    printf("Keep pushing forward! Excellence awaits!\n\n");
}

// MAIN FUNCTION
int main() {
    srand((unsigned int)time(NULL));
    
    loadConfig();
    
    consoleWindow = GetConsoleWindow();
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Set console properties
    SetWindowPos(consoleWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    char title[128];
    sprintf(title, "UNIVERSAL STUDY REMINDER - %s", config.subject);
    SetConsoleTitle(title);
    
    if(config.enableOpenGL) {
        initOpenGL();
    }
    
    // Startup sequence
    system("cls");
    setRandomColor();
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              ║\n");
    printf("║         UNIVERSAL STUDY REMINDER SYSTEM                     ║\n");
    printf("║              MAXIMUM OVERKILL EDITION                       ║\n");
    printf("║                                                              ║\n");
    printf("║  Subject: %-50s ║\n", config.subject);
    printf("║  Interval: %d minutes                                       ║\n", config.reminderInterval);
    printf("║  Messages loaded: %d                                        ║\n", config.messageCount);
    printf("║  Apps to launch: %d                                         ║\n", config.appCount);
    printf("║                                                              ║\n");
    printf("║  Edit 'study_reminder.conf' to customize                    ║\n");
    printf("║  Press Ctrl+C to stop                                       ║\n");
    printf("║                                                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    // Countdown
    for(int i = 3; i > 0; i--) {
        setRandomColor();
        printf("ACTIVATING IN %d...\n", i);
        if(config.enableSound) Beep(600 + i*100, 300);
        Sleep(1000);
    }
    
    printf("\nSYSTEM ACTIVATED! READY TO DOMINATE YOUR STUDIES!\n\n");
    
    while(1) {
        Sleep(config.reminderInterval * 60 * 1000);
        launchUniversalNotification();
    }
    
    return 0;
}
