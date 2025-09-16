#include "ottsr.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "winmm.lib")

static ottsr_app_t g_app = {0};

void ottsr_init_app(ottsr_app_t* app) {
    memset(app, 0, sizeof(ottsr_app_t));
    app->session.state = OTTSR_STATE_IDLE;
    app->session.profile_index = 0;
    app->config.theme = OTTSR_THEME_LIGHT;
    app->config.sound_volume = 50;
    app->config.minimize_to_tray = 1;
    app->config.autostart_sessions = 0;
    
    // Create default profile
    strcpy_s(app->config.profiles[0].name, sizeof(app->config.profiles[0].name), "Default");
    app->config.profiles[0].study_minutes = 25;
    app->config.profiles[0].break_minutes = 5;
    app->config.profiles[0].long_break_minutes = 15;
    app->config.profiles[0].sessions_until_long_break = 4;
    app->config.profiles[0].sound_enabled = 1;
    app->config.profiles[0].notifications_enabled = 1;
    app->config.profile_count = 1;
    
    ottsr_load_config(app);
}

void ottsr_cleanup_app(ottsr_app_t* app) {
    ottsr_save_config(app);
    
    if (app->title_font) DeleteObject(app->title_font);
    if (app->normal_font) DeleteObject(app->normal_font);
    if (app->mono_font) DeleteObject(app->mono_font);
    if (app->bg_brush) DeleteObject(app->bg_brush);
    if (app->card_brush) DeleteObject(app->card_brush);
    if (app->primary_brush) DeleteObject(app->primary_brush);
    if (app->border_pen) DeleteObject(app->border_pen);
}

void ottsr_load_config(ottsr_app_t* app) {
    FILE* file;
    if (fopen_s(&file, OTTSR_CONFIG_FILE, "r") != 0) {
        return; // Use defaults
    }
    
    char line[512];
    ottsr_profile_t* current_profile = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; // Remove newlines
        
        if (line[0] == '#' || strlen(line) == 0) continue;
        
        char key[128], value[384];
        if (sscanf_s(line, "%127[^=]=%383[^\n]", key, (unsigned)sizeof(key), value, (unsigned)sizeof(value)) == 2) {
            if (strcmp(key, "theme") == 0) {
                app->config.theme = atoi(value);
            } else if (strcmp(key, "minimize_to_tray") == 0) {
                app->config.minimize_to_tray = atoi(value);
            } else if (strcmp(key, "autostart_sessions") == 0) {
                app->config.autostart_sessions = atoi(value);
            } else if (strcmp(key, "sound_volume") == 0) {
                app->config.sound_volume = atoi(value);
            } else if (strcmp(key, "profile_count") == 0) {
                app->config.profile_count = min(atoi(value), OTTSR_MAX_PROFILES);
            } else if (strcmp(key, "active_profile") == 0) {
                app->config.active_profile = min(atoi(value), app->config.profile_count - 1);
            } else if (strncmp(key, "profile_", 8) == 0) {
                int profile_idx = atoi(key + 8);
                if (profile_idx < OTTSR_MAX_PROFILES) {
                    current_profile = &app->config.profiles[profile_idx];
                    char* field = strchr(key + 8, '_');
                    if (field++) {
                        if (strcmp(field, "name") == 0) {
                            strcpy_s(current_profile->name, sizeof(current_profile->name), value);
                        } else if (strcmp(field, "study_minutes") == 0) {
                            current_profile->study_minutes = atoi(value);
                        } else if (strcmp(field, "break_minutes") == 0) {
                            current_profile->break_minutes = atoi(value);
                        } else if (strcmp(field, "long_break_minutes") == 0) {
                            current_profile->long_break_minutes = atoi(value);
                        } else if (strcmp(field, "sessions_until_long_break") == 0) {
                            current_profile->sessions_until_long_break = atoi(value);
                        } else if (strcmp(field, "sound_enabled") == 0) {
                            current_profile->sound_enabled = atoi(value);
                        } else if (strcmp(field, "notifications_enabled") == 0) {
                            current_profile->notifications_enabled = atoi(value);
                        } else if (strcmp(field, "total_study_time") == 0) {
                            current_profile->total_study_time = (time_t)_atoi64(value);
                        } else if (strcmp(field, "total_sessions") == 0) {
                            current_profile->total_sessions = atoi(value);
                        } else if (strcmp(field, "completed_sessions") == 0) {
                            current_profile->completed_sessions = atoi(value);
                        }
                    }
                }
            }
        }
    }
    
    fclose(file);
}

void ottsr_save_config(ottsr_app_t* app) {
    FILE* file;
    if (fopen_s(&file, OTTSR_CONFIG_FILE, "w") != 0) {
        return;
    }
    
    fprintf(file, "# Over The Top Study Reminder Configuration\n");
    fprintf(file, "# Generated automatically - edit carefully\n\n");
    
    fprintf(file, "theme=%d\n", app->config.theme);
    fprintf(file, "minimize_to_tray=%d\n", app->config.minimize_to_tray);
    fprintf(file, "autostart_sessions=%d\n", app->config.autostart_sessions);
    fprintf(file, "sound_volume=%d\n", app->config.sound_volume);
    fprintf(file, "profile_count=%d\n", app->config.profile_count);
    fprintf(file, "active_profile=%d\n", app->config.active_profile);
    
    for (int i = 0; i < app->config.profile_count; i++) {
        ottsr_profile_t* p = &app->config.profiles[i];
        fprintf(file, "\nprofile_%d_name=%s\n", i, p->name);
        fprintf(file, "profile_%d_study_minutes=%d\n", i, p->study_minutes);
        fprintf(file, "profile_%d_break_minutes=%d\n", i, p->break_minutes);
        fprintf(file, "profile_%d_long_break_minutes=%d\n", i, p->long_break_minutes);
        fprintf(file, "profile_%d_sessions_until_long_break=%d\n", i, p->sessions_until_long_break);
        fprintf(file, "profile_%d_sound_enabled=%d\n", i, p->sound_enabled);
        fprintf(file, "profile_%d_notifications_enabled=%d\n", i, p->notifications_enabled);
        fprintf(file, "profile_%d_total_study_time=%lld\n", i, (long long)p->total_study_time);
        fprintf(file, "profile_%d_total_sessions=%d\n", i, p->total_sessions);
        fprintf(file, "profile_%d_completed_sessions=%d\n", i, p->completed_sessions);
    }
    
    fclose(file);
}

void ottsr_create_ui_resources(ottsr_app_t* app) {
    app->title_font = CreateFont(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    
    app->normal_font = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    
    app->mono_font = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    
    ottsr_update_ui_theme(app);
}

void ottsr_update_ui_theme(ottsr_app_t* app) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    app->is_dark_theme = (app->config.theme == OTTSR_THEME_DARK) ||
        (app->config.theme == OTTSR_THEME_AUTO && (st.wHour < 6 || st.wHour > 18));
    
    if (app->bg_brush) DeleteObject(app->bg_brush);
    if (app->card_brush) DeleteObject(app->card_brush);
    if (app->primary_brush) DeleteObject(app->primary_brush);
    if (app->border_pen) DeleteObject(app->border_pen);
    
    app->bg_brush = CreateSolidBrush(app->is_dark_theme ? OTTSR_COLOR_BG_DARK : OTTSR_COLOR_BG_LIGHT);
    app->card_brush = CreateSolidBrush(app->is_dark_theme ? OTTSR_COLOR_CARD_DARK : OTTSR_COLOR_CARD_LIGHT);
    app->primary_brush = CreateSolidBrush(OTTSR_COLOR_PRIMARY);
    app->border_pen = CreatePen(PS_SOLID, 1, app->is_dark_theme ? RGB(75, 85, 99) : RGB(209, 213, 219));
}

void ottsr_create_main_window(ottsr_app_t* app, HINSTANCE hInstance) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = ottsr_main_wndproc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OTTSRMainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = app->bg_brush;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClass(&wc);
    
    app->main_window = CreateWindow(L"OTTSRMainWindow",
        L"Over The Top Study Reminder " OTTSR_VERSION,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 520,
        NULL, NULL, hInstance, app);
    
    // Create controls
    CreateWindow(L"STATIC", L"Profile:", WS_VISIBLE | WS_CHILD,
        20, 20, 60, 25, app->main_window, NULL, hInstance, NULL);
    
    app->profile_combo = CreateWindow(L"COMBOBOX", NULL,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        90, 18, 200, 200, app->main_window, (HMENU)IDC_PROFILE_COMBO, hInstance, NULL);
    
    CreateWindow(L"STATIC", L"Subject:", WS_VISIBLE | WS_CHILD,
        20, 55, 60, 25, app->main_window, NULL, hInstance, NULL);
    
    app->subject_edit = CreateWindow(L"EDIT", NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        90, 53, 300, 25, app->main_window, (HMENU)IDC_SUBJECT_EDIT, hInstance, NULL);
    
    CreateWindow(L"STATIC", L"Study Time (min):", WS_VISIBLE | WS_CHILD,
        20, 90, 120, 25, app->main_window, NULL, hInstance, NULL);
    
    app->study_time_edit = CreateWindow(L"EDIT", NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        150, 88, 60, 25, app->main_window, (HMENU)IDC_STUDY_TIME_EDIT, hInstance, NULL);
    
    CreateWindow(L"STATIC", L"Break Time (min):", WS_VISIBLE | WS_CHILD,
        230, 90, 120, 25, app->main_window, NULL, hInstance, NULL);
    
    app->break_time_edit = CreateWindow(L"EDIT", NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        360, 88, 60, 25, app->main_window, (HMENU)IDC_BREAK_TIME_EDIT, hInstance, NULL);
    
    // Session controls
    app->start_btn = CreateWindow(L"BUTTON", L"Start Session",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 130, 100, 35, app->main_window, (HMENU)IDC_START_BTN, hInstance, NULL);
    
    app->pause_btn = CreateWindow(L"BUTTON", L"Pause",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        130, 130, 100, 35, app->main_window, (HMENU)IDC_PAUSE_BTN, hInstance, NULL);
    
    app->stop_btn = CreateWindow(L"BUTTON", L"Stop",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        240, 130, 100, 35, app->main_window, (HMENU)IDC_STOP_BTN, hInstance, NULL);
    
    // Progress and status
    CreateWindow(L"STATIC", L"Session Progress:", WS_VISIBLE | WS_CHILD,
        20, 185, 120, 20, app->main_window, NULL, hInstance, NULL);
    
    app->session_progress = CreateWindow(PROGRESS_CLASS, NULL,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        20, 205, 420, 20, app->main_window, (HMENU)IDC_SESSION_PROGRESS, hInstance, NULL);
    
    CreateWindow(L"STATIC", L"Break Progress:", WS_VISIBLE | WS_CHILD,
        20, 235, 120, 20, app->main_window, NULL, hInstance, NULL);
    
    app->break_progress = CreateWindow(PROGRESS_CLASS, NULL,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        20, 255, 420, 20, app->main_window, (HMENU)IDC_BREAK_PROGRESS, hInstance, NULL);
    
    app->time_display = CreateWindow(L"STATIC", L"00:00",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 295, 420, 60, app->main_window, (HMENU)IDC_TIME_DISPLAY, hInstance, NULL);
    
    app->status_display = CreateWindow(L"STATIC", L"Ready to study",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 365, 420, 25, app->main_window, (HMENU)IDC_STATUS_DISPLAY, hInstance, NULL);
    
    // Bottom buttons
    CreateWindow(L"BUTTON", L"Settings",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 410, 80, 30, app->main_window, (HMENU)IDC_SETTINGS_BTN, hInstance, NULL);
    
    CreateWindow(L"BUTTON", L"Profiles",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        110, 410, 80, 30, app->main_window, (HMENU)IDC_PROFILES_BTN, hInstance, NULL);
    
    CreateWindow(L"BUTTON", L"About",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        360, 410, 80, 30, app->main_window, (HMENU)IDC_ABOUT_BTN, hInstance, NULL);
    
    // Initialize UI state
    EnableWindow(app->pause_btn, FALSE);
    EnableWindow(app->stop_btn, FALSE);
    
    // Populate profile combo
    for (int i = 0; i < app->config.profile_count; i++) {
        SendMessage(app->profile_combo, CB_ADDSTRING, 0, (LPARAM)app->config.profiles[i].name);
    }
    SendMessage(app->profile_combo, CB_SETCURSEL, app->config.active_profile, 0);
    
    // Set fonts
    SendMessage(app->time_display, WM_SETFONT, (WPARAM)app->mono_font, TRUE);
    
    ottsr_update_display(app);
}

void ottsr_start_session(ottsr_app_t* app) {
    if (app->session.state != OTTSR_STATE_IDLE) return;
    
    // Get current profile
    int profile_idx = (int)SendMessage(app->profile_combo, CB_GETCURSEL, 0, 0);
    if (profile_idx == CB_ERR) profile_idx = 0;
    
    app->session.profile_index = profile_idx;
    app->session.state = OTTSR_STATE_STUDYING;
    app->session.session_start = time(NULL);
    app->session.elapsed_study_seconds = 0;
    app->session.elapsed_break_seconds = 0;
    
    // Get subject
    GetWindowText(app->subject_edit, app->session.current_subject, OTTSR_MAX_NAME_LEN);
    
    // Update profile stats
    app->config.profiles[profile_idx].total_sessions++;
    
    // Start timer
    SetTimer(app->main_window, TIMER_SESSION, 1000, NULL);
    SetTimer(app->main_window, TIMER_UI_UPDATE, 100, NULL);
    
    // Update UI
    EnableWindow(app->start_btn, FALSE);
    EnableWindow(app->pause_btn, TRUE);
    EnableWindow(app->stop_btn, TRUE);
    SetWindowText(app->status_display, L"Studying...");
    
    ottsr_update_display(app);
}

void ottsr_stop_session(ottsr_app_t* app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    KillTimer(app->main_window, TIMER_SESSION);
    KillTimer(app->main_window, TIMER_BREAK);
    KillTimer(app->main_window, TIMER_UI_UPDATE);
    
    // Update profile stats
    ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
    profile->total_study_time += app->session.elapsed_study_seconds;
    
    if (app->session.state == OTTSR_STATE_STUDYING && 
        app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
        profile->completed_sessions++;
    }
    
    // Reset session
    app->session.state = OTTSR_STATE_IDLE;
    app->session.current_sessions = 0;
    
    // Update UI
    EnableWindow(app->start_btn, TRUE);
    EnableWindow(app->pause_btn, FALSE);
    EnableWindow(app->stop_btn, FALSE);
    SetWindowText(app->status_display, L"Session completed");
    
    SendMessage(app->session_progress, PBM_SETPOS, 0, 0);
    SendMessage(app->break_progress, PBM_SETPOS, 0, 0);
    
    ottsr_save_config(app);
    ottsr_update_display(app);
}

void ottsr_pause_session(ottsr_app_t* app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    if (app->session.state == OTTSR_STATE_PAUSED) {
        // Resume
        time_t pause_duration = time(NULL) - app->session.pause_start;
        app->session.session_start += pause_duration;
        if (app->session.break_start > 0) {
            app->session.break_start += pause_duration;
        }
        
        if (app->session.state == OTTSR_STATE_STUDYING) {
            SetTimer(app->main_window, TIMER_SESSION, 1000, NULL);
        } else {
            SetTimer(app->main_window, TIMER_BREAK, 1000, NULL);
        }
        
        app->session.state = (app->session.break_start > 0) ? OTTSR_STATE_BREAKING : OTTSR_STATE_STUDYING;
        SetWindowText(app->pause_btn, L"Pause");
        SetWindowText(app->status_display, 
            (app->session.state == OTTSR_STATE_STUDYING) ? L"Studying..." : L"Break time...");
    } else {
        // Pause
        KillTimer(app->main_window, TIMER_SESSION);
        KillTimer(app->main_window, TIMER_BREAK);
        
        app->session.pause_start = time(NULL);
        app->session.state = OTTSR_STATE_PAUSED;
        SetWindowText(app->pause_btn, L"Resume");
        SetWindowText(app->status_display, L"Paused");
    }
    
    ottsr_update_display(app);
}

void ottsr_update_display(ottsr_app_t* app) {
    ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
    
    // Update time display
    char time_str[32];
    int display_seconds = 0;
    
    if (app->session.state == OTTSR_STATE_STUDYING) {
        display_seconds = (profile->study_minutes * 60) - app->session.elapsed_study_seconds;
        int progress = (app->session.elapsed_study_seconds * 100) / (profile->study_minutes * 60);
        SendMessage(app->session_progress, PBM_SETPOS, max(0, min(100, progress)), 0);
        SendMessage(app->break_progress, PBM_SETPOS, 0, 0);
    } else if (app->session.state == OTTSR_STATE_BREAKING) {
        int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
            profile->long_break_minutes : profile->break_minutes;
        display_seconds = (break_minutes * 60) - app->session.elapsed_break_seconds;
        int progress = (app->session.elapsed_break_seconds * 100) / (break_minutes * 60);
        SendMessage(app->break_progress, PBM_SETPOS, max(0, min(100, progress)), 0);
    } else {
        display_seconds = profile->study_minutes * 60;
        SendMessage(app->session_progress, PBM_SETPOS, 0, 0);
        SendMessage(app->break_progress, PBM_SETPOS, 0, 0);
    }
    
    ottsr_format_time(max(0, display_seconds), time_str, sizeof(time_str));
    SetWindowTextA(app->time_display, time_str);
    
    // Update profile-specific controls
    char buffer[16];
    sprintf_s(buffer, sizeof(buffer), "%d", profile->study_minutes);
    SetWindowTextA(app->study_time_edit, buffer);
    
    sprintf_s(buffer, sizeof(buffer), "%d", profile->break_minutes);
    SetWindowTextA(app->break_time_edit, buffer);
}

void ottsr_handle_timer(ottsr_app_t* app, WPARAM timer_id) {
    ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
    
    switch (timer_id) {
    case TIMER_SESSION:
        app->session.elapsed_study_seconds++;
        
        if (app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
            // Study session complete, start break
            KillTimer(app->main_window, TIMER_SESSION);
            app->session.current_sessions++;
            app->session.state = OTTSR_STATE_BREAKING;
            app->session.break_start = time(NULL);
            app->session.elapsed_break_seconds = 0;
            
            SetTimer(app->main_window, TIMER_BREAK, 1000, NULL);
            SetWindowText(app->status_display, L"Break time!");
            
            if (profile->notifications_enabled) {
                int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
                    profile->long_break_minutes : profile->break_minutes;
                char msg[256];
                sprintf_s(msg, sizeof(msg), "Study session complete! Take a %d minute break.", break_minutes);
                ottsr_show_notification("Study Complete", msg);
            }
            
            if (profile->sound_enabled) {
                ottsr_play_notification_sound(app);
            }
        }
        break;
        
    case TIMER_BREAK:
        app->session.elapsed_break_seconds++;
        
        int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
            profile->long_break_minutes : profile->break_minutes;
            
        if (app->session.elapsed_break_seconds >= break_minutes * 60) {
            // Break complete
            KillTimer(app->main_window, TIMER_BREAK);
            
            if (app->config.autostart_sessions) {
                // Auto-start next session
                app->session.state = OTTSR_STATE_STUDYING;
                app->session.session_start = time(NULL);
                app->session.elapsed_study_seconds = 0;
                SetTimer(app->main_window, TIMER_SESSION, 1000, NULL);
                SetWindowText(app->status_display, L"Studying...");
            } else {
                // Wait for manual start
                app->session.state = OTTSR_STATE_IDLE;
                EnableWindow(app->start_btn, TRUE);
                EnableWindow(app->pause_btn, FALSE);
                EnableWindow(app->stop_btn, FALSE);
                SetWindowText(app->status_display, L"Break complete - ready for next session");
            }
            
            if (profile->notifications_enabled) {
                ottsr_show_notification("Break Complete", "Ready for the next study session!");
            }
            
            if (profile->sound_enabled) {
                ottsr_play_notification_sound(app);
            }
        }
        break;
        
    case TIMER_UI_UPDATE:
        ottsr_update_display(app);
        break;
    }
}

void ottsr_format_time(int seconds, char* buffer, size_t buffer_size) {
    int minutes = seconds / 60;
    seconds = seconds % 60;
    sprintf_s(buffer, buffer_size, "%02d:%02d", minutes, seconds);
}

void ottsr_play_notification_sound(ottsr_app_t* app) {
    int volume = app->config.sound_volume;
    for (int i = 0; i < 3; i++) {
        Beep(800 + (i * 200), 200);
        Sleep(100);
    }
}

void ottsr_show_notification(const char* title, const char* message) {
    MessageBoxA(NULL, message, title, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
}

LRESULT CALLBACK ottsr_main_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ottsr_app_t* app = (ottsr_app_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
    case WM_CREATE:
        {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return 0;
        }
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_START_BTN:
            ottsr_start_session(app);
            break;
            
        case IDC_PAUSE_BTN:
            ottsr_pause_session(app);
            break;
            
        case IDC_STOP_BTN:
            ottsr_stop_session(app);
            break;
            
        case IDC_SETTINGS_BTN:
            ottsr_show_settings_dialog(app);
            break;
            
        case IDC_PROFILES_BTN:
            ottsr_show_profiles_dialog(app);
            break;
            
        case IDC_ABOUT_BTN:
            ottsr_show_about_dialog(app);
            break;
            
        case IDC_PROFILE_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE && app->session.state == OTTSR_STATE_IDLE) {
                app->config.active_profile = (int)SendMessage(app->profile_combo, CB_GETCURSEL, 0, 0);
                app->session.profile_index = app->config.active_profile;
                ottsr_update_display(app);
            }
            break;
            
        case IDC_STUDY_TIME_EDIT:
        case IDC_BREAK_TIME_EDIT:
            if (HIWORD(wParam) == EN_CHANGE && app->session.state == OTTSR_STATE_IDLE) {
                char buffer[16];
                ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
                
                if (LOWORD(wParam) == IDC_STUDY_TIME_EDIT) {
                    GetWindowTextA(app->study_time_edit, buffer, sizeof(buffer));
                    int value = atoi(buffer);
                    if (value > 0 && value <= 180) {
                        profile->study_minutes = value;
                    }
                } else {
                    GetWindowTextA(app->break_time_edit, buffer, sizeof(buffer));
                    int value = atoi(buffer);
                    if (value > 0 && value <= 60) {
                        profile->break_minutes = value;
                    }
                }
            }
            break;
        }
        return 0;
        
    case WM_TIMER:
        ottsr_handle_timer(app, wParam);
        return 0;
        
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        if (app && app->is_dark_theme) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, OTTSR_COLOR_TEXT_DARK);
            SetBkColor(hdc, app->is_dark_theme ? OTTSR_COLOR_CARD_DARK : OTTSR_COLOR_CARD_LIGHT);
            return (LRESULT)(app->is_dark_theme ? app->card_brush : app->bg_brush);
        }
        break;
        
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (app) {
                FillRect(hdc, &ps.rcPaint, app->bg_brush);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
    case WM_CLOSE:
        if (app->session.state != OTTSR_STATE_IDLE) {
            int result = MessageBox(hwnd, 
                L"A study session is active. Do you want to save and exit?",
                L"Confirm Exit", MB_YESNOCANCEL | MB_ICONQUESTION);
            
            if (result == IDYES) {
                ottsr_stop_session(app);
            } else if (result == IDCANCEL) {
                return 0;
            }
        }
        
        ottsr_cleanup_app(app);
        DestroyWindow(hwnd);
        return 0;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ottsr_show_settings_dialog(ottsr_app_t* app) {
    char settings_msg[1024];
    sprintf_s(settings_msg, sizeof(settings_msg),
        "Current Settings:\n\n"
        "Theme: %s\n"
        "Sound Volume: %d%%\n"
        "Minimize to Tray: %s\n"
        "Auto-start Sessions: %s\n\n"
        "Edit %s to modify advanced settings.",
        (app->config.theme == OTTSR_THEME_LIGHT) ? "Light" : 
        (app->config.theme == OTTSR_THEME_DARK) ? "Dark" : "Auto",
        app->config.sound_volume,
        app->config.minimize_to_tray ? "Yes" : "No",
        app->config.autostart_sessions ? "Yes" : "No",
        OTTSR_CONFIG_FILE);
    
    MessageBoxA(app->main_window, settings_msg, "Settings", MB_OK | MB_ICONINFORMATION);
}

void ottsr_show_profiles_dialog(ottsr_app_t* app) {
    char profiles_msg[2048] = "Study Profiles:\n\n";
    
    for (int i = 0; i < app->config.profile_count; i++) {
        ottsr_profile_t* p = &app->config.profiles[i];
        char profile_info[512];
        
        int total_hours = (int)(p->total_study_time / 3600);
        int total_minutes = (int)((p->total_study_time % 3600) / 60);
        
        sprintf_s(profile_info, sizeof(profile_info),
            "%s%s:\n"
            "  Study: %dm, Break: %dm, Long Break: %dm\n"
            "  Total Time: %dh %dm\n"
            "  Sessions: %d completed / %d total\n\n",
            p->name,
            (i == app->config.active_profile) ? " [ACTIVE]" : "",
            p->study_minutes, p->break_minutes, p->long_break_minutes,
            total_hours, total_minutes,
            p->completed_sessions, p->total_sessions);
        
        strcat_s(profiles_msg, sizeof(profiles_msg), profile_info);
    }
    
    MessageBoxA(app->main_window, profiles_msg, "Study Profiles", MB_OK | MB_ICONINFORMATION);
}

void ottsr_show_about_dialog(ottsr_app_t* app) {
    MessageBoxA(app->main_window,
        "Over The Top Study Reminder v" OTTSR_VERSION "\n\n"
        "A professional study timer with customizable profiles,\n"
        "break reminders, and session tracking.\n\n"
        "Features:\n"
        "- Multiple study profiles\n"
        "- Pomodoro technique support\n" 
        "- Session statistics tracking\n"
        "- Modern, clean interface\n"
        "- Configurable notifications\n\n"
        "Credits:\n"
        "Developed by g-flame\n"
        "GitHub: github.com/g-flame\n\n"
        "Copyright 2024 - MIT License",
        "About OTTSR", MB_OK | MB_ICONINFORMATION);
}

INT_PTR CALLBACK ottsr_settings_dlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Settings dialog implementation would go here
    // For now, using simple message boxes
    return FALSE;
}

INT_PTR CALLBACK ottsr_profiles_dlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Profiles dialog implementation would go here
    // For now, using simple message boxes  
    return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    ottsr_init_app(&g_app);
    ottsr_create_ui_resources(&g_app);
    ottsr_create_main_window(&g_app, hInstance);
    
    ShowWindow(g_app.main_window, nCmdShow);
    UpdateWindow(g_app.main_window);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
