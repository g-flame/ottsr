#include "ottsr.h"

static ottsr_app_t g_app = {0};

// Forward declarations for dialog templates
BOOL CALLBACK CreateSettingsDialog(HWND parent, ottsr_app_t* app);
BOOL CALLBACK CreateProfilesDialog(HWND parent, ottsr_app_t* app);

void ottsr_init_app(ottsr_app_t* app) {
    memset(app, 0, sizeof(ottsr_app_t));
    app->session.state = OTTSR_STATE_IDLE;
    app->session.profile_index = 0;
    app->config.theme = OTTSR_THEME_LIGHT;
    app->config.sound_volume = 70;
    app->config.minimize_to_tray = 1;
    app->config.autostart_sessions = 0;
    app->config.window_x = CW_USEDEFAULT;
    app->config.window_y = CW_USEDEFAULT;
    
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
}

void ottsr_load_config(ottsr_app_t* app) {
    FILE* file;
    if (fopen_s(&file, OTTSR_CONFIG_FILE, "r") != 0) {
        return; // Use defaults
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        
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
            } else if (strcmp(key, "window_x") == 0) {
                app->config.window_x = atoi(value);
            } else if (strcmp(key, "window_y") == 0) {
                app->config.window_y = atoi(value);
            } else if (strcmp(key, "last_subject") == 0) {
                strcpy_s(app->config.last_subject, sizeof(app->config.last_subject), value);
            } else if (strncmp(key, "profile_", 8) == 0) {
                int profile_idx = atoi(key + 8);
                if (profile_idx < OTTSR_MAX_PROFILES) {
                    char* field = strchr(key + 8, '_');
                    if (field++) {
                        ottsr_profile_t* p = &app->config.profiles[profile_idx];
                        if (strcmp(field, "name") == 0) {
                            strcpy_s(p->name, sizeof(p->name), value);
                        } else if (strcmp(field, "study_minutes") == 0) {
                            p->study_minutes = atoi(value);
                        } else if (strcmp(field, "break_minutes") == 0) {
                            p->break_minutes = atoi(value);
                        } else if (strcmp(field, "long_break_minutes") == 0) {
                            p->long_break_minutes = atoi(value);
                        } else if (strcmp(field, "sessions_until_long_break") == 0) {
                            p->sessions_until_long_break = atoi(value);
                        } else if (strcmp(field, "sound_enabled") == 0) {
                            p->sound_enabled = atoi(value);
                        } else if (strcmp(field, "notifications_enabled") == 0) {
                            p->notifications_enabled = atoi(value);
                        } else if (strcmp(field, "total_study_time") == 0) {
                            p->total_study_time = (time_t)_atoi64(value);
                        } else if (strcmp(field, "total_sessions") == 0) {
                            p->total_sessions = atoi(value);
                        } else if (strcmp(field, "completed_sessions") == 0) {
                            p->completed_sessions = atoi(value);
                        }
                    }
                }
            }
        }
    }
    
    fclose(file);
}

void ottsr_save_config(ottsr_app_t* app) {
    // Save window position
    if (app->main_window) {
        RECT rect;
        GetWindowRect(app->main_window, &rect);
        app->config.window_x = rect.left;
        app->config.window_y = rect.top;
    }
    
    // Save current subject
    if (app->subject_edit) {
        GetWindowTextA(app->subject_edit, app->config.last_subject, OTTSR_MAX_NAME_LEN);
    }
    
    FILE* file;
    if (fopen_s(&file, OTTSR_CONFIG_FILE, "w") != 0) {
        return;
    }
    
    fprintf(file, "# Over The Top Study Reminder Configuration\n");
    fprintf(file, "# Version: %s\n\n", OTTSR_VERSION);
    
    fprintf(file, "theme=%d\n", app->config.theme);
    fprintf(file, "minimize_to_tray=%d\n", app->config.minimize_to_tray);
    fprintf(file, "autostart_sessions=%d\n", app->config.autostart_sessions);
    fprintf(file, "sound_volume=%d\n", app->config.sound_volume);
    fprintf(file, "profile_count=%d\n", app->config.profile_count);
    fprintf(file, "active_profile=%d\n", app->config.active_profile);
    fprintf(file, "window_x=%d\n", app->config.window_x);
    fprintf(file, "window_y=%d\n", app->config.window_y);
    fprintf(file, "last_subject=%s\n", app->config.last_subject);
    
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

void ottsr_create_main_window(ottsr_app_t* app, HINSTANCE hInstance) {
    // Create UI resources
    app->title_font = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    
    app->normal_font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    
    app->mono_font = CreateFontA(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    
    app->bg_brush = CreateSolidBrush(COLOR_BG_LIGHT);
    app->card_brush = CreateSolidBrush(COLOR_CARD_LIGHT);
    app->primary_brush = CreateSolidBrush(COLOR_PRIMARY);
    
    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = ottsr_main_wndproc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "OTTSRMainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = app->bg_brush;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassA(&wc);
    
    // Create main window
    app->main_window = CreateWindowA("OTTSRMainWindow",
        "Over The Top Study Reminder v" OTTSR_VERSION,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        app->config.window_x, app->config.window_y, 520, 600,
        NULL, NULL, hInstance, app);
    
    // Profile section
    CreateWindowA("STATIC", "Study Profile:", WS_VISIBLE | WS_CHILD,
        20, 20, 100, 20, app->main_window, NULL, hInstance, NULL);
    
    app->profile_combo = CreateWindowA("COMBOBOX", NULL,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        20, 45, 200, 200, app->main_window, (HMENU)IDC_PROFILE_COMBO, hInstance, NULL);
    
    CreateWindowA("BUTTON", "Manage Profiles",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        240, 45, 120, 25, app->main_window, (HMENU)IDC_PROFILES_BTN, hInstance, NULL);
    
    // Subject section
    CreateWindowA("STATIC", "Current Subject:", WS_VISIBLE | WS_CHILD,
        20, 85, 100, 20, app->main_window, NULL, hInstance, NULL);
    
    app->subject_edit = CreateWindowA("EDIT", app->config.last_subject,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        20, 105, 340, 25, app->main_window, (HMENU)IDC_SUBJECT_EDIT, hInstance, NULL);
    
    // Time settings
    CreateWindowA("STATIC", "Study Time (minutes):", WS_VISIBLE | WS_CHILD,
        20, 145, 140, 20, app->main_window, NULL, hInstance, NULL);
    
    app->study_time_edit = CreateWindowA("EDIT", "25",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        20, 165, 60, 25, app->main_window, (HMENU)IDC_STUDY_TIME_EDIT, hInstance, NULL);
    
    CreateWindowA("STATIC", "Break Time (minutes):", WS_VISIBLE | WS_CHILD,
        100, 145, 140, 20, app->main_window, NULL, hInstance, NULL);
    
    app->break_time_edit = CreateWindowA("EDIT", "5",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        100, 165, 60, 25, app->main_window, (HMENU)IDC_BREAK_TIME_EDIT, hInstance, NULL);
    
    // Session controls
    app->start_btn = CreateWindowA("BUTTON", "Start Session",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
        20, 210, 100, 40, app->main_window, (HMENU)IDC_START_BTN, hInstance, NULL);
    
    app->pause_btn = CreateWindowA("BUTTON", "Pause",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        130, 210, 80, 40, app->main_window, (HMENU)IDC_PAUSE_BTN, hInstance, NULL);
    
    app->stop_btn = CreateWindowA("BUTTON", "Stop",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        220, 210, 80, 40, app->main_window, (HMENU)IDC_STOP_BTN, hInstance, NULL);
    
    // Timer display
    app->time_display = CreateWindowA("STATIC", "25:00",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 270, 460, 80, app->main_window, (HMENU)IDC_TIME_DISPLAY, hInstance, NULL);
    
    // Status display
    app->status_display = CreateWindowA("STATIC", "Ready to start studying",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 360, 460, 25, app->main_window, (HMENU)IDC_STATUS_DISPLAY, hInstance, NULL);
    
    // Progress bars
    CreateWindowA("STATIC", "Session Progress:", WS_VISIBLE | WS_CHILD,
        20, 400, 120, 20, app->main_window, NULL, hInstance, NULL);
    
    app->session_progress = CreateWindowA(PROGRESS_CLASSA, NULL,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        20, 420, 460, 15, app->main_window, (HMENU)IDC_SESSION_PROGRESS, hInstance, NULL);
    
    CreateWindowA("STATIC", "Break Progress:", WS_VISIBLE | WS_CHILD,
        20, 450, 120, 20, app->main_window, NULL, hInstance, NULL);
    
    app->break_progress = CreateWindowA(PROGRESS_CLASSA, NULL,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        20, 470, 460, 15, app->main_window, (HMENU)IDC_BREAK_PROGRESS, hInstance, NULL);
    
    // Bottom buttons
    CreateWindowA("BUTTON", "Settings",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 510, 80, 30, app->main_window, (HMENU)IDC_SETTINGS_BTN, hInstance, NULL);
    
    CreateWindowA("BUTTON", "About",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        400, 510, 80, 30, app->main_window, (HMENU)IDC_ABOUT_BTN, hInstance, NULL);
    
    // Set fonts
    SendMessage(app->time_display, WM_SETFONT, (WPARAM)app->mono_font, TRUE);
    SendMessage(app->profile_combo, WM_SETFONT, (WPARAM)app->normal_font, TRUE);
    SendMessage(app->subject_edit, WM_SETFONT, (WPARAM)app->normal_font, TRUE);
    
    // Initialize UI state
    EnableWindow(app->pause_btn, FALSE);
    EnableWindow(app->stop_btn, FALSE);
    
    // Populate profile combo
    for (int i = 0; i < app->config.profile_count; i++) {
        SendMessageA(app->profile_combo, CB_ADDSTRING, 0, (LPARAM)app->config.profiles[i].name);
    }
    SendMessage(app->profile_combo, CB_SETCURSEL, app->config.active_profile, 0);
    
    ottsr_update_display(app);
}

void ottsr_start_session(ottsr_app_t* app) {
    if (app->session.state != OTTSR_STATE_IDLE) return;
    
    int profile_idx = (int)SendMessage(app->profile_combo, CB_GETCURSEL, 0, 0);
    if (profile_idx == CB_ERR) profile_idx = 0;
    
    app->session.profile_index = profile_idx;
    app->session.state = OTTSR_STATE_STUDYING;
    app->session.session_start = time(NULL);
    app->session.elapsed_study_seconds = 0;
    app->session.elapsed_break_seconds = 0;
    app->session.current_sessions = 0;
    
    GetWindowTextA(app->subject_edit, app->session.current_subject, OTTSR_MAX_NAME_LEN);
    
    app->config.profiles[profile_idx].total_sessions++;
    
    SetTimer(app->main_window, TIMER_SESSION, 1000, NULL);
    SetTimer(app->main_window, TIMER_UI_UPDATE, 100, NULL);
    
    EnableWindow(app->start_btn, FALSE);
    EnableWindow(app->pause_btn, TRUE);
    EnableWindow(app->stop_btn, TRUE);
    SetWindowTextA(app->status_display, "Studying...");
    
    ottsr_update_display(app);
}

void ottsr_stop_session(ottsr_app_t* app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    KillTimer(app->main_window, TIMER_SESSION);
    KillTimer(app->main_window, TIMER_BREAK);
    KillTimer(app->main_window, TIMER_UI_UPDATE);
    
    ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
    profile->total_study_time += app->session.elapsed_study_seconds;
    
    if (app->session.state == OTTSR_STATE_STUDYING && 
        app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
        profile->completed_sessions++;
    }
    
    app->session.state = OTTSR_STATE_IDLE;
    app->session.current_sessions = 0;
    
    EnableWindow(app->start_btn, TRUE);
    EnableWindow(app->pause_btn, FALSE);
    EnableWindow(app->stop_btn, FALSE);
    SetWindowTextA(app->pause_btn, "Pause");
    SetWindowTextA(app->status_display, "Session completed");
    
    SendMessage(app->session_progress, PBM_SETPOS, 0, 0);
    SendMessage(app->break_progress, PBM_SETPOS, 0, 0);
    
    ottsr_save_config(app);
    ottsr_update_display(app);
}

void ottsr_pause_session(ottsr_app_t* app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    if (app->session.state == OTTSR_STATE_PAUSED) {
        time_t pause_duration = time(NULL) - app->session.pause_start;
        app->session.session_start += pause_duration;
        if (app->session.break_start > 0) {
            app->session.break_start += pause_duration;
        }
        
        if (app->session.break_start > 0) {
            app->session.state = OTTSR_STATE_BREAKING;
            SetTimer(app->main_window, TIMER_BREAK, 1000, NULL);
            SetWindowTextA(app->status_display, "Break time...");
        } else {
            app->session.state = OTTSR_STATE_STUDYING;
            SetTimer(app->main_window, TIMER_SESSION, 1000, NULL);
            SetWindowTextA(app->status_display, "Studying...");
        }
        
        SetWindowTextA(app->pause_btn, "Pause");
    } else {
        KillTimer(app->main_window, TIMER_SESSION);
        KillTimer(app->main_window, TIMER_BREAK);
        
        app->session.pause_start = time(NULL);
        app->session.state = OTTSR_STATE_PAUSED;
        SetWindowTextA(app->pause_btn, "Resume");
        SetWindowTextA(app->status_display, "Paused");
    }
    
    ottsr_update_display(app);
}

void ottsr_update_display(ottsr_app_t* app) {
    ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
    
    char time_str[32];
    int display_seconds = 0;
    
    if (app->session.state == OTTSR_STATE_STUDYING || app->session.state == OTTSR_STATE_PAUSED) {
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
        SendMessage(app->session_progress, PBM_SETPOS, 100, 0);
    } else {
        display_seconds = profile->study_minutes * 60;
        SendMessage(app->session_progress, PBM_SETPOS, 0, 0);
        SendMessage(app->break_progress, PBM_SETPOS, 0, 0);
    }
    
    ottsr_format_time(max(0, display_seconds), time_str, sizeof(time_str));
    SetWindowTextA(app->time_display, time_str);
    
    // Update time input fields
    if (app->session.state == OTTSR_STATE_IDLE) {
        char buffer[16];
        sprintf_s(buffer, sizeof(buffer), "%d", profile->study_minutes);
        SetWindowTextA(app->study_time_edit, buffer);
        
        sprintf_s(buffer, sizeof(buffer), "%d", profile->break_minutes);
        SetWindowTextA(app->break_time_edit, buffer);
    }
}

void ottsr_handle_timer(ottsr_app_t* app, WPARAM timer_id) {
    ottsr_profile_t* profile = &app->config.profiles[app->session.profile_index];
    
    switch (timer_id) {
    case TIMER_SESSION:
        app->session.elapsed_study_seconds++;
        
        if (app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
            KillTimer(app->main_window, TIMER_SESSION);
            app->session.current_sessions++;
            app->session.state = OTTSR_STATE_BREAKING;
            app->session.break_start = time(NULL);
            app->session.elapsed_break_seconds = 0;
            
            SetTimer(app->main_window, TIMER_BREAK, 1000, NULL);
            SetWindowTextA(app->status_display, "Break time!");
            
            if (profile->notifications_enabled) {
                int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
                    profile->long_break_minutes : profile->break_minutes;
                char msg[256];
                sprintf_s(msg, sizeof(msg), "Study session complete! Take a %d minute break.", break_minutes);
                MessageBoxA(app->main_window, msg, "Study Complete", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
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
            KillTimer(app->main_window, TIMER_BREAK);
            
            if (app->config.autostart_sessions) {
                app->session.state = OTTSR_STATE_STUDYING;
                app->session.session_start = time(NULL);
                app->session.elapsed_study_seconds = 0;
                app->session.break_start = 0;
                SetTimer(app->main_window, TIMER_SESSION, 1000, NULL);
                SetWindowTextA(app->status_display, "Next session started automatically");
            } else {
                app->session.state = OTTSR_STATE_IDLE;
                app->session.break_start = 0;
                EnableWindow(app->start_btn, TRUE);
                EnableWindow(app->pause_btn, FALSE);
                EnableWindow(app->stop_btn, FALSE);
                SetWindowTextA(app->pause_btn, "Pause");
                SetWindowTextA(app->status_display, "Break complete - ready for next session");
            }
            
            if (profile->notifications_enabled) {
                MessageBoxA(app->main_window, "Break complete! Ready for the next study session.", 
                          "Break Complete", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
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
    for (int i = 0; i < 3; i++) {
        Beep(800 + (i * 200), 200);
        Sleep(100);
    }
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
                ottsr_update_display(app);
            }
            break;
        }
        return 0;
        
    case WM_TIMER:
        ottsr_handle_timer(app, wParam);
        return 0;
        
    case WM_CLOSE:
        if (app->session.state != OTTSR_STATE_IDLE) {
            int result = MessageBoxA(hwnd, 
                "A study session is active. Do you want to save and exit?",
                "Confirm Exit", MB_YESNOCANCEL | MB_ICONQUESTION);
            
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
        "To modify settings, edit the %s file manually.\n"
        "Advanced settings dialog coming in future updates.",
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
            "  Study: %d min, Break: %d min, Long Break: %d min\n"
            "  Sessions until long break: %d\n"
            "  Total study time: %dh %dm\n"
            "  Completed sessions: %d / %d total\n\n",
            p->name,
            (i == app->config.active_profile) ? " [ACTIVE]" : "",
            p->study_minutes, p->break_minutes, p->long_break_minutes,
            p->sessions_until_long_break,
            total_hours, total_minutes,
            p->completed_sessions, p->total_sessions);
        
        strcat_s(profiles_msg, sizeof(profiles_msg), profile_info);
    }
    
    strcat_s(profiles_msg, sizeof(profiles_msg), 
        "\nTo add/edit profiles, modify the configuration file manually.\n"
        "Full profile editor coming in future updates.");
    
    MessageBoxA(app->main_window, profiles_msg, "Study Profiles", MB_OK | MB_ICONINFORMATION);
}

void ottsr_show_about_dialog(ottsr_app_t* app) {
    MessageBoxA(app->main_window,
        "Over The Top Study Reminder v" OTTSR_VERSION "\n\n"
        "A professional study timer application with customizable\n"
        "profiles, break reminders, and session tracking.\n\n"
        "Key Features:\n"
        "• Multiple study profiles with individual settings\n"
        "• Pomodoro technique support with configurable intervals\n" 
        "• Session statistics and progress tracking\n"
        "• Clean, modern user interface\n"
        "• Configurable audio notifications\n"
        "• Automatic session management\n"
        "• Persistent configuration storage\n\n"
        "Credits:\n"
        "Developed by g-flame\n"
        "GitHub: https://github.com/g-flame\n\n"
        "This software is open source and free to use.\n"
        "Copyright 2024 - MIT License",
        "About OTTSR", MB_OK | MB_ICONINFORMATION);
}

INT_PTR CALLBACK ottsr_settings_dlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

INT_PTR CALLBACK ottsr_profiles_dlgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    ottsr_init_app(&g_app);
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
