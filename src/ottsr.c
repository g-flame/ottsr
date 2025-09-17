#include "ottsr.h"
#include <stddef.h>

static ottsr_app_t g_app = {0};

// Forward declarations
static void ottsr_activate(GtkApplication *app, gpointer user_data);

// Application startup
static void ottsr_activate(GtkApplication *app, gpointer user_data) {
    ottsr_app_t *ottsr_app = (ottsr_app_t *)user_data;
    ottsr_app->app = app;
    
    ottsr_init_app(ottsr_app);
    ottsr_create_main_window(ottsr_app);
    
    gtk_widget_show_all(ottsr_app->main_window);
}

// Initialize application state
void ottsr_init_app(ottsr_app_t *app) {
    memset(app, 0, sizeof(ottsr_app_t));
    
    // Initialize session
    app->session.state = OTTSR_STATE_IDLE;
    app->session.profile_index = 0;
    
    // Initialize config with defaults
    app->config.theme = OTTSR_THEME_LIGHT;
    app->config.sound_volume = 70;
    app->config.minimize_to_tray = TRUE;
    app->config.autostart_sessions = FALSE;
    app->config.window_width = OTTSR_WINDOW_WIDTH;
    app->config.window_height = OTTSR_WINDOW_HEIGHT;
    
    // Create default profile
    strcpy(app->config.profiles[0].name, "Pomodoro");
    app->config.profiles[0].study_minutes = 25;
    app->config.profiles[0].break_minutes = 5;
    app->config.profiles[0].long_break_minutes = 15;
    app->config.profiles[0].sessions_until_long_break = 4;
    app->config.profiles[0].sound_enabled = TRUE;
    app->config.profiles[0].notifications_enabled = TRUE;
    
    strcpy(app->config.profiles[1].name, "Deep Work");
    app->config.profiles[1].study_minutes = 90;
    app->config.profiles[1].break_minutes = 20;
    app->config.profiles[1].long_break_minutes = 30;
    app->config.profiles[1].sessions_until_long_break = 2;
    app->config.profiles[1].sound_enabled = TRUE;
    app->config.profiles[1].notifications_enabled = TRUE;
    
    strcpy(app->config.profiles[2].name, "Short Sprint");
    app->config.profiles[2].study_minutes = 15;
    app->config.profiles[2].break_minutes = 3;
    app->config.profiles[2].long_break_minutes = 10;
    app->config.profiles[2].sessions_until_long_break = 3;
    app->config.profiles[2].sound_enabled = TRUE;
    app->config.profiles[2].notifications_enabled = TRUE;
    
    app->config.profile_count = 3;
    app->config.active_profile = 0;
    
    // Load saved config
    if (!ottsr_load_config(app)) {
        g_print("Using default configuration\n");
    }
}

// Create main window with modern design
void ottsr_create_main_window(ottsr_app_t *app) {
    GError *error = NULL;
    
    // Create main window
    app->main_window = gtk_application_window_new(app->app);
    gtk_window_set_title(GTK_WINDOW(app->main_window), "Study Timer Pro");
    gtk_window_set_default_size(GTK_WINDOW(app->main_window), 
                               app->config.window_width, 
                               app->config.window_height);
    gtk_window_set_resizable(GTK_WINDOW(app->main_window), FALSE);
    
    // Load CSS styling
    app->css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(app->css_provider, OTTSR_CSS_STYLE, -1, &error);
    if (error) {
        g_warning("Error loading CSS: %s", error->message);
        g_error_free(error);
    } else {
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(app->css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    
    // Create main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->main_window), main_box);
    
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_style_context_add_class(gtk_widget_get_style_context(container), "main-container");
    gtk_box_pack_start(GTK_BOX(main_box), container, TRUE, TRUE, 0);
    
    // Header with title
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(container), header_box, FALSE, FALSE, 0);
    
    GtkWidget *title_label = gtk_label_new("Study Timer Pro");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "timer-display");
    gtk_box_pack_start(GTK_BOX(header_box), title_label, FALSE, FALSE, 0);
    
    // Profile selection
    GtkWidget *profile_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(container), profile_box, FALSE, FALSE, 0);
    
    GtkWidget *profile_label = gtk_label_new("Profile:");
    gtk_box_pack_start(GTK_BOX(profile_box), profile_label, FALSE, FALSE, 0);
    
    app->profile_combo = gtk_combo_box_text_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(app->profile_combo), "profile-combo");
    for (int i = 0; i < app->config.profile_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->profile_combo), 
                                      app->config.profiles[i].name);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->profile_combo), app->config.active_profile);
    g_signal_connect(app->profile_combo, "changed", G_CALLBACK(on_profile_changed), app);
    gtk_box_pack_start(GTK_BOX(profile_box), app->profile_combo, TRUE, TRUE, 0);
    
    GtkWidget *profiles_btn = gtk_button_new_with_label("Manage");
    gtk_style_context_add_class(gtk_widget_get_style_context(profiles_btn), "control-button");
    g_signal_connect(profiles_btn, "clicked", G_CALLBACK(on_profiles_clicked), app);
    gtk_box_pack_start(GTK_BOX(profile_box), profiles_btn, FALSE, FALSE, 0);
    
    // Subject entry
    GtkWidget *subject_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(container), subject_box, FALSE, FALSE, 0);
    
    GtkWidget *subject_label = gtk_label_new("What are you studying?");
    gtk_widget_set_halign(subject_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(subject_box), subject_label, FALSE, FALSE, 0);
    
    app->subject_entry = gtk_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(app->subject_entry), "settings-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->subject_entry), "Enter your subject here...");
    gtk_entry_set_text(GTK_ENTRY(app->subject_entry), app->config.last_subject);
    g_signal_connect(app->subject_entry, "changed", G_CALLBACK(on_subject_changed), app);
    gtk_box_pack_start(GTK_BOX(subject_box), app->subject_entry, FALSE, FALSE, 0);
    
    // Time settings
    GtkWidget *time_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(time_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(time_grid), 20);
    gtk_widget_set_halign(time_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(container), time_grid, FALSE, FALSE, 0);
    
    // Study time
    GtkWidget *study_label = gtk_label_new("Study (min):");
    gtk_grid_attach(GTK_GRID(time_grid), study_label, 0, 0, 1, 1);
    
    app->study_time_spin = gtk_spin_button_new_with_range(1, 180, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->study_time_spin), 
                             app->config.profiles[app->config.active_profile].study_minutes);
    g_signal_connect(app->study_time_spin, "value-changed", G_CALLBACK(on_time_changed), app);
    gtk_grid_attach(GTK_GRID(time_grid), app->study_time_spin, 1, 0, 1, 1);
    
    // Break time
    GtkWidget *break_label = gtk_label_new("Break (min):");
    gtk_grid_attach(GTK_GRID(time_grid), break_label, 2, 0, 1, 1);
    
    app->break_time_spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->break_time_spin), 
                             app->config.profiles[app->config.active_profile].break_minutes);
    g_signal_connect(app->break_time_spin, "value-changed", G_CALLBACK(on_time_changed), app);
    gtk_grid_attach(GTK_GRID(time_grid), app->break_time_spin, 3, 0, 1, 1);
    
    // Timer display
    app->timer_label = gtk_label_new("25:00");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->timer_label), "timer-display");
    gtk_widget_set_margin_top(app->timer_label, 20);
    gtk_widget_set_margin_bottom(app->timer_label, 20);
    gtk_box_pack_start(GTK_BOX(container), app->timer_label, FALSE, FALSE, 0);
    
    // Status label
    app->status_label = gtk_label_new("Ready to start studying");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->status_label), "status-label");
    gtk_box_pack_start(GTK_BOX(container), app->status_label, FALSE, FALSE, 0);
    
    // Control buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(container), button_box, FALSE, FALSE, 0);
    
    app->start_button = gtk_button_new_with_label("Start Session");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->start_button), "control-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->start_button), "start-button");
    g_signal_connect(app->start_button, "clicked", G_CALLBACK(on_start_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), app->start_button, FALSE, FALSE, 0);
    
    app->pause_button = gtk_button_new_with_label("Pause");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->pause_button), "control-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->pause_button), "pause-button");
    gtk_widget_set_sensitive(app->pause_button, FALSE);
    g_signal_connect(app->pause_button, "clicked", G_CALLBACK(on_pause_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), app->pause_button, FALSE, FALSE, 0);
    
    app->stop_button = gtk_button_new_with_label("Stop");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->stop_button), "control-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->stop_button), "stop-button");
    gtk_widget_set_sensitive(app->stop_button, FALSE);
    g_signal_connect(app->stop_button, "clicked", G_CALLBACK(on_stop_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), app->stop_button, FALSE, FALSE, 0);
    
    // Progress bars
    GtkWidget *progress_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(progress_box, 20);
    gtk_box_pack_start(GTK_BOX(container), progress_box, FALSE, FALSE, 0);
    
    GtkWidget *session_label = gtk_label_new("Session Progress");
    gtk_widget_set_halign(session_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(progress_box), session_label, FALSE, FALSE, 0);
    
    app->session_progress = gtk_progress_bar_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(app->session_progress), "progress-bar");
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(app->session_progress), TRUE);
    gtk_box_pack_start(GTK_BOX(progress_box), app->session_progress, FALSE, FALSE, 0);
    
    GtkWidget *break_label_prog = gtk_label_new("Break Progress");
    gtk_widget_set_halign(break_label_prog, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(progress_box), break_label_prog, FALSE, FALSE, 0);
    
    app->break_progress = gtk_progress_bar_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(app->break_progress), "progress-bar");
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(app->break_progress), TRUE);
    gtk_box_pack_start(GTK_BOX(progress_box), app->break_progress, FALSE, FALSE, 0);
    
    // Stats display
    app->stats_label = gtk_label_new("");
    gtk_widget_set_margin_top(app->stats_label, 20);
    gtk_box_pack_start(GTK_BOX(container), app->stats_label, FALSE, FALSE, 0);
    
    // Bottom buttons
    GtkWidget *bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(bottom_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(bottom_box, 20);
    gtk_box_pack_start(GTK_BOX(container), bottom_box, FALSE, FALSE, 0);
    
    GtkWidget *settings_btn = gtk_button_new_with_label("Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "control-button");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), app);
    gtk_box_pack_start(GTK_BOX(bottom_box), settings_btn, FALSE, FALSE, 0);
    
    GtkWidget *about_btn = gtk_button_new_with_label("About");
    gtk_style_context_add_class(gtk_widget_get_style_context(about_btn), "control-button");
    g_signal_connect(about_btn, "clicked", G_CALLBACK(on_about_clicked), app);
    gtk_box_pack_start(GTK_BOX(bottom_box), about_btn, FALSE, FALSE, 0);
    
    // Initial display update
    ottsr_update_display(app);
}

// Session management
void ottsr_start_session(ottsr_app_t *app) {
    if (app->session.state != OTTSR_STATE_IDLE) return;
    
    int profile_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(app->profile_combo));
    if (profile_idx < 0) profile_idx = 0;
    
    app->session.profile_index = profile_idx;
    app->session.state = OTTSR_STATE_STUDYING;
    app->session.session_start = time(NULL);
    app->session.elapsed_study_seconds = 0;
    app->session.elapsed_break_seconds = 0;
    app->session.current_sessions = 0;
    
    const char *subject = gtk_entry_get_text(GTK_ENTRY(app->subject_entry));
    strncpy(app->session.current_subject, subject, OTTSR_MAX_NAME_LEN - 1);
    app->session.current_subject[OTTSR_MAX_NAME_LEN - 1] = '\0';
    
    app->config.profiles[profile_idx].total_sessions++;
    
    // Start timers
    app->session_timer_id = g_timeout_add_seconds(1, ottsr_timer_callback, app);
    app->ui_update_timer_id = g_timeout_add(100, ottsr_ui_update_callback, app);
    
    // Update UI
    gtk_widget_set_sensitive(app->start_button, FALSE);
    gtk_widget_set_sensitive(app->pause_button, TRUE);
    gtk_widget_set_sensitive(app->stop_button, TRUE);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Studying...");
    
    ottsr_update_display(app);
}

void ottsr_stop_session(ottsr_app_t *app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    // Stop timers
    if (app->session_timer_id > 0) {
        g_source_remove(app->session_timer_id);
        app->session_timer_id = 0;
    }
    if (app->ui_update_timer_id > 0) {
        g_source_remove(app->ui_update_timer_id);
        app->ui_update_timer_id = 0;
    }
    
    // Update statistics
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    profile->total_study_time += app->session.elapsed_study_seconds;
    
    if (app->session.state == OTTSR_STATE_STUDYING && 
        app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
        profile->completed_sessions++;
    }
    
    // Reset state
    app->session.state = OTTSR_STATE_IDLE;
    app->session.current_sessions = 0;
    
    // Update UI
    gtk_widget_set_sensitive(app->start_button, TRUE);
    gtk_widget_set_sensitive(app->pause_button, FALSE);
    gtk_widget_set_sensitive(app->stop_button, FALSE);
    gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
    gtk_label_set_text(GTK_LABEL(app->status_label), "Session completed");
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), 0.0);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->break_progress), 0.0);
    
    ottsr_save_config(app);
    ottsr_update_display(app);
}

// Update display with current state
void ottsr_update_display(ottsr_app_t *app) {
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    char time_str[32];
    int display_seconds = profile->study_minutes * 60;
    
    // Format time as MM:SS
    int minutes = display_seconds / 60;
    int seconds = display_seconds % 60;
    snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
    
    gtk_label_set_text(GTK_LABEL(app->timer_label), time_str);
    
    // Update time spinners
    g_signal_handlers_block_by_func(app->study_time_spin, on_time_changed, app);
    g_signal_handlers_block_by_func(app->break_time_spin, on_time_changed, app);
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->study_time_spin), profile->study_minutes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->break_time_spin), profile->break_minutes);
    
    g_signal_handlers_unblock_by_func(app->study_time_spin, on_time_changed, app);
    g_signal_handlers_unblock_by_func(app->break_time_spin, on_time_changed, app);
    
    // Update stats
    char stats_str[256];
    int total_hours = (int)(profile->total_study_time / 3600);
    int total_minutes = (int)((profile->total_study_time % 3600) / 60);
    
    snprintf(stats_str, sizeof(stats_str), 
             "Total Study Time: %dh %dm | Sessions: %d/%d completed",
             total_hours, total_minutes, 
             profile->completed_sessions, profile->total_sessions);
    
    gtk_label_set_text(GTK_LABEL(app->stats_label), stats_str);
}

// Timer callback
gboolean ottsr_timer_callback(gpointer user_data) {
    ottsr_app_t *app = (ottsr_app_t *)user_data;
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    if (app->session.state == OTTSR_STATE_STUDYING) {
        app->session.elapsed_study_seconds++;
        
        if (app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
            // Study session complete
            app->session.state = OTTSR_STATE_IDLE;
            gtk_widget_set_sensitive(app->start_button, TRUE);
            gtk_widget_set_sensitive(app->pause_button, FALSE);
            gtk_widget_set_sensitive(app->stop_button, FALSE);
            gtk_label_set_text(GTK_LABEL(app->status_label), "Study session complete! Take a break.");
            
            profile->completed_sessions++;
            profile->total_study_time += app->session.elapsed_study_seconds;
            
            return G_SOURCE_REMOVE; // Stop timer
        }
    }
    
    return G_SOURCE_CONTINUE;
}

// UI update callback
gboolean ottsr_ui_update_callback(gpointer user_data) {
    ottsr_app_t *app = (ottsr_app_t *)user_data;
    
    if (app->session.state == OTTSR_STATE_STUDYING) {
        ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
        int remaining = (profile->study_minutes * 60) - app->session.elapsed_study_seconds;
        
        char time_str[32];
        int minutes = remaining / 60;
        int seconds = remaining % 60;
        snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
        
        gtk_label_set_text(GTK_LABEL(app->timer_label), time_str);
        
        // Update progress
        double progress = (double)app->session.elapsed_study_seconds / (profile->study_minutes * 60);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), progress);
    }
    
    return G_SOURCE_CONTINUE;
}

// Callback implementations
void on_profile_changed(GtkComboBox *combo, ottsr_app_t *app) {
    if (app->session.state != OTTSR_STATE_IDLE) return;
    
    int new_profile = gtk_combo_box_get_active(combo);
    if (new_profile >= 0 && new_profile < app->config.profile_count) {
        app->config.active_profile = new_profile;
        app->session.profile_index = new_profile;
        ottsr_update_display(app);
    }
}

void on_time_changed(GtkSpinButton *spin, ottsr_app_t *app) {
    if (app->session.state != OTTSR_STATE_IDLE) return;
    
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    if (spin == GTK_SPIN_BUTTON(app->study_time_spin)) {
        profile->study_minutes = gtk_spin_button_get_value_as_int(spin);
    } else if (spin == GTK_SPIN_BUTTON(app->break_time_spin)) {
        profile->break_minutes = gtk_spin_button_get_value_as_int(spin);
    }
    
    ottsr_update_display(app);
}

void on_subject_changed(GtkEntry *entry, ottsr_app_t *app) {
    const char *text = gtk_entry_get_text(entry);
    strncpy(app->config.last_subject, text, OTTSR_MAX_NAME_LEN - 1);
    app->config.last_subject[OTTSR_MAX_NAME_LEN - 1] = '\0';
}

void on_about_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->main_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Study Timer Pro v2.0.0\n\nA modern study timer with GTK3\n\nBuilt for productive studying!");
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
// Dummy implementations for unfinished functions
gboolean ottsr_load_config(ottsr_app_t *app) { return FALSE; }
gboolean ottsr_save_config(ottsr_app_t *app) { return TRUE; }

void ottsr_pause_session(ottsr_app_t *app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    if (app->session.state == OTTSR_STATE_PAUSED) {
        // Resume
        app->session.state = OTTSR_STATE_STUDYING;
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
        gtk_label_set_text(GTK_LABEL(app->status_label), "Studying...");
    } else {
        // Pause
        app->session.state = OTTSR_STATE_PAUSED;
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Resume");
        gtk_label_set_text(GTK_LABEL(app->status_label), "Paused");
    }
}

void ottsr_cleanup_app(ottsr_app_t *app) {
    if (app->session_timer_id > 0) {
        g_source_remove(app->session_timer_id);
    }
    if (app->ui_update_timer_id > 0) {
        g_source_remove(app->ui_update_timer_id);
    }
    
    if (app->css_provider) {
        g_object_unref(app->css_provider);
    }
}

void on_start_clicked(GtkButton *button, ottsr_app_t *app) { ottsr_start_session(app); }
void on_pause_clicked(GtkButton *button, ottsr_app_t *app) { ottsr_pause_session(app); }
void on_stop_clicked(GtkButton *button, ottsr_app_t *app) { ottsr_stop_session(app); }

void on_settings_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->main_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Settings dialog not implemented yet.\n\nYou can adjust study and break times using the spinners above.");
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_profiles_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->main_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Profile manager not implemented yet.\n\nCurrently using the selected profile from the dropdown.");
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void ottsr_create_settings_window(ottsr_app_t *app) {
    // Stub - not implemented yet
}

void ottsr_create_profiles_window(ottsr_app_t *app) {
    // Stub - not implemented yet  
}

// Main function
int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("com.github.g-flame.ottsr", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(ottsr_activate), &g_app);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
    ottsr_cleanup_app(&g_app);
    g_object_unref(app);
    
    return status;
}
