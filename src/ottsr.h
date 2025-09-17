#ifndef OTTSR_H
#define OTTSR_H

#include <gtk/gtk.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Application Constants
#define OTTSR_VERSION "2.0.0"
#define OTTSR_CONFIG_DIR ".config/ottsr"
#define OTTSR_CONFIG_FILE "settings.json"
#define OTTSR_MAX_PROFILES 20
#define OTTSR_MAX_NAME_LEN 128
#define OTTSR_WINDOW_WIDTH 480
#define OTTSR_WINDOW_HEIGHT 720

// CSS for modern styling
#define OTTSR_CSS_STYLE \
"window { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }" \
".main-container { background: rgba(255, 255, 255, 0.95); border-radius: 20px; " \
"                  margin: 20px; padding: 30px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); }" \
".timer-display { font-family: 'SF Mono', 'Monaco', 'Cascadia Code', monospace; " \
"                 font-size: 48px; font-weight: bold; color: #2c3e50; " \
"                 text-shadow: 0 2px 4px rgba(0,0,0,0.1); }" \
".status-label { font-size: 18px; color: #7f8c8d; margin: 10px 0; }" \
".profile-combo { padding: 12px; border-radius: 8px; font-size: 14px; }" \
".control-button { padding: 12px 24px; border-radius: 25px; font-weight: 600; " \
"                  font-size: 14px; transition: all 0.3s ease; }" \
".start-button { background: linear-gradient(45deg, #27ae60, #2ecc71); color: white; border: none; }" \
".start-button:hover { background: linear-gradient(45deg, #229954, #27ae60); transform: translateY(-2px); }" \
".pause-button { background: linear-gradient(45deg, #f39c12, #e67e22); color: white; border: none; }" \
".pause-button:hover { background: linear-gradient(45deg, #e67e22, #d35400); transform: translateY(-2px); }" \
".stop-button { background: linear-gradient(45deg, #e74c3c, #c0392b); color: white; border: none; }" \
".stop-button:hover { background: linear-gradient(45deg, #c0392b, #a93226); transform: translateY(-2px); }" \
".progress-bar { border-radius: 10px; }" \
".progress-bar progress { background: linear-gradient(90deg, #667eea, #764ba2); }" \
".settings-entry { padding: 8px 12px; border-radius: 6px; margin: 5px 0; }"

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
    gboolean sound_enabled;
    gboolean notifications_enabled;
    time_t total_study_time;
    int total_sessions;
    int completed_sessions;
} ottsr_profile_t;

typedef struct {
    ottsr_profile_t profiles[OTTSR_MAX_PROFILES];
    int profile_count;
    int active_profile;
    ottsr_theme_t theme;
    gboolean minimize_to_tray;
    gboolean autostart_sessions;
    int sound_volume;
    int window_width;
    int window_height;
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
    GtkApplication *app;
    GtkWidget *main_window;
    GtkWidget *settings_window;
    GtkWidget *profiles_window;
    
    // Main window widgets
    GtkWidget *profile_combo;
    GtkWidget *subject_entry;
    GtkWidget *study_time_spin;
    GtkWidget *break_time_spin;
    GtkWidget *start_button;
    GtkWidget *pause_button;
    GtkWidget *stop_button;
    GtkWidget *timer_label;
    GtkWidget *status_label;
    GtkWidget *session_progress;
    GtkWidget *break_progress;
    GtkWidget *stats_label;
    
    // Settings widgets
    GtkWidget *theme_combo;
    GtkWidget *volume_scale;
    GtkWidget *sound_check;
    GtkWidget *notifications_check;
    GtkWidget *minimize_check;
    GtkWidget *autostart_check;
    
    // Profile widgets
    GtkWidget *profile_list;
    GtkWidget *profile_name_entry;
    GtkWidget *profile_study_spin;
    GtkWidget *profile_break_spin;
    GtkWidget *profile_longbreak_spin;
    GtkWidget *profile_sessions_spin;
    
    // Timers
    guint session_timer_id;
    guint ui_update_timer_id;
    
    // State
    ottsr_config_t config;
    ottsr_session_t session;
    
    // Styling
    GtkCssProvider *css_provider;
} ottsr_app_t;

// Function declarations
void ottsr_init_app(ottsr_app_t *app);
void ottsr_cleanup_app(ottsr_app_t *app);
gboolean ottsr_load_config(ottsr_app_t *app);
gboolean ottsr_save_config(ottsr_app_t *app);
void ottsr_create_main_window(ottsr_app_t *app);
void ottsr_create_settings_window(ottsr_app_t *app);
void ottsr_create_profiles_window(ottsr_app_t *app);
void ottsr_start_session(ottsr_app_t *app);
void ottsr_stop_session(ottsr_app_t *app);
void ottsr_pause_session(ottsr_app_t *app);
void ottsr_update_display(ottsr_app_t *app);
gboolean ottsr_timer_callback(gpointer user_data);
gboolean ottsr_ui_update_callback(gpointer user_data);
void ottsr_show_notification(ottsr_app_t *app, const char *title, const char *message);
void ottsr_play_notification_sound(ottsr_app_t *app);
void ottsr_format_time(int seconds, char *buffer, size_t buffer_size);
void ottsr_format_stats(ottsr_app_t *app, char *buffer, size_t buffer_size);
char* ottsr_get_config_path(void);

// Callback declarations
void on_profile_changed(GtkComboBox *combo, ottsr_app_t *app);
void on_start_clicked(GtkButton *button, ottsr_app_t *app);
void on_pause_clicked(GtkButton *button, ottsr_app_t *app);
void on_stop_clicked(GtkButton *button, ottsr_app_t *app);
void on_settings_clicked(GtkButton *button, ottsr_app_t *app);
void on_profiles_clicked(GtkButton *button, ottsr_app_t *app);
void on_about_clicked(GtkButton *button, ottsr_app_t *app);
void on_time_changed(GtkSpinButton *spin, ottsr_app_t *app);
void on_subject_changed(GtkEntry *entry, ottsr_app_t *app);

// Settings callbacks
void on_settings_save_clicked(GtkButton *button, ottsr_app_t *app);
void on_settings_cancel_clicked(GtkButton *button, ottsr_app_t *app);

// Profile callbacks
void on_profile_add_clicked(GtkButton *button, ottsr_app_t *app);
void on_profile_delete_clicked(GtkButton *button, ottsr_app_t *app);
void on_profile_save_clicked(GtkButton *button, ottsr_app_t *app);
void on_profile_cancel_clicked(GtkButton *button, ottsr_app_t *app);
void on_profile_list_changed(GtkListBox *list, GtkListBoxRow *row, ottsr_app_t *app);

#endif // OTTSR_H
