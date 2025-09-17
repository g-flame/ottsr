#ifndef OTTSR_H
#define OTTSR_H

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "winmm.lib")

// Application Constants
#define OTTSR_VERSION "1.0.0"
#define OTTSR_CONFIG_FILE "ottsr.conf"
#define OTTSR_MAX_PROFILES 20
#define OTTSR_MAX_NAME_LEN 128

// UI Control IDs
#define IDC_PROFILE_COMBO       2001
#define IDC_SUBJECT_EDIT        2002
#define IDC_STUDY_TIME_EDIT     2003
#define IDC_BREAK_TIME_EDIT     2004
#define IDC_START_BTN           2005
#define IDC_STOP_BTN            2006
#define IDC_PAUSE_BTN           2007
#define IDC_SETTINGS_BTN        2008
#define IDC_PROFILES_BTN        2009
#define IDC_ABOUT_BTN           2010
#define IDC_SESSION_PROGRESS    2011
#define IDC_BREAK_PROGRESS      2012
#define IDC_TIME_DISPLAY        2013
#define IDC_STATUS_DISPLAY      2014

// Settings Dialog IDs
#define IDD_SETTINGS            3000
#define IDC_SOUND_CHECK         3001
#define IDC_NOTIFICATIONS_CHECK 3002
#define IDC_MINIMIZE_CHECK      3003
#define IDC_AUTOSTART_CHECK     3004
#define IDC_VOLUME_SLIDER       3005
#define IDC_THEME_COMBO         3006
#define IDC_SAVE_SETTINGS       3007
#define IDC_RESET_SETTINGS      3008

// Profile Dialog IDs  
#define IDD_PROFILES            4000
#define IDC_PROFILE_LIST        4001
#define IDC_PROFILE_NAME_EDIT   4002
#define IDC_ADD_PROFILE         4003
#define IDC_DELETE_PROFILE      4004
#define IDC_EDIT_PROFILE        4005
#define IDC_PROFILE_STUDY_EDIT  4006
#define IDC_PROFILE_BREAK_EDIT  4007
#define IDC_PROFILE_LONGBREAK_EDIT 4008
#define IDC_PROFILE_SESSIONS_EDIT  4009

// Timer IDs
#define TIMER_SESSION           1
#define TIMER_BREAK            2
#define TIMER_UI_UPDATE        3

// Colors
#define COLOR_PRIMARY     RGB(59, 130, 246)
#define COLOR_SECONDARY   RGB(107, 114, 128)
#define COLOR_SUCCESS     RGB(34, 197, 94)
#define COLOR_WARNING     RGB(245, 158, 11)
#define COLOR_ERROR       RGB(239, 68, 68)
#define COLOR_BG_LIGHT    RGB(249, 250, 251)
#define COLOR_BG_DARK     RGB(17, 24, 39)
#define COLOR_CARD_LIGHT  RGB(255, 255, 255)
#define COLOR_CARD_DARK   RGB(31, 41, 55)

// Enums
typedef enum {
    OTTSR_STATE_IDLE,
    OTTSR_STATE_STUDYING,
    OTTSR_STATE_BREAKING,
    OTTSR_STATE_PAUSED
} ottsr_state_t;

typedef enum {
    OTTSR_THEME_LIGHT,
    OTTSR_THEME_DARK,
    OTTSR_THEME_AUTO
} ottsr_theme_t;

// Structures
typedef struct {
    char name[OTTSR_MAX_NAME_LEN];
    int study_minutes;
    int break_minutes;
    int long_break_minutes;
    int sessions_until_long_break;
    int sound_enabled;
    int notifications_enabled;
    time_t total_study_time;
    int total_sessions;
    int completed_sessions;
} ottsr_profile_t;

typedef struct {
    ottsr_profile_t profiles[OTTSR_MAX_PROFILES];
    int profile_count;
    int active_profile;
    ottsr_theme_t theme;
    int minimize_to_tray;
    int autostart_sessions;
    int sound_volume;
    int window_x;
    int window_y;
    char last_subject[OTTSR_MAX_NAME_LEN];
} ottsr_config_t;

typedef struct {
    ottsr_state_t state;
    time_t session_start;
    time_t break_start;
    time_t pause_start;
    int elapsed_study_seconds;
    int elapsed_break_seconds;
    int current_sessions;
    char current_subject[OTTSR_MAX_NAME_LEN];
    int profile_index;
} ottsr_session_t;

typedef struct {
    HWND main_window;
    HWND settings_dialog;
    HWND profiles_dialog;
    
    // Main window controls
    HWND profile_combo;
    HWND subject_edit;
    HWND study_time_edit;
    HWND break_time_edit;
    HWND start_btn;
    HWND stop_btn;
    HWND pause_btn;
    HWND session_progress;
    HWND break_progress;
    HWND time_display;
    HWND status_display;
    
    // UI Resources
    HFONT title_font;
    HFONT normal_font;
    HFONT mono_font;
    HBRUSH bg_brush;
    HBRUSH card_brush;
    HBRUSH primary_brush;
    
    // State
    ottsr_config_t config;
    ottsr_session_t session;
    int is_dark_theme;
} ottsr_app_t;

// Function declarations
void ottsr_init_app(ottsr_app_t* app);
void ottsr_cleanup_app(ottsr_app_t* app);
void ottsr_load_config(ottsr_app_t* app);
void ottsr_save_config(ottsr_app_t* app);
void ottsr_create_main_window(ottsr_app_t* app, HINSTANCE hInstance);
void ottsr_start_session(ottsr_app_t* app);
void ottsr_stop_session(ottsr_app_t* app);
void ottsr_pause_session(ottsr_app_t* app);
void ottsr_update_display(ottsr_app_t* app);
void ottsr_handle_timer(ottsr_app_t* app, WPARAM timer_id);
void ottsr_show_settings_dialog(ottsr_app_t* app);
void ottsr_show_profiles_dialog(ottsr_app_t* app);
void ottsr_show_about_dialog(ottsr_app_t* app);
LRESULT CALLBACK ottsr_main_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ottsr_settings_dlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ottsr_profiles_dlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ottsr_format_time(int seconds, char* buffer, size_t buffer_size);
void ottsr_play_notification_sound(ottsr_app_t* app);

#endif // OTTSR_H
