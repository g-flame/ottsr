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
    
    if (ottsr_app->main_window) {
        gtk_widget_show_all(ottsr_app->main_window);
    }
}

// Initialize application state
void ottsr_init_app(ottsr_app_t *app) {
    memset(app, 0, sizeof(ottsr_app_t));
    
    // Initialize session
    app->session.state = OTTSR_STATE_IDLE;
    app->session.profile_index = 0;
    app->session.pause_duration = 0;
    app->session.is_long_break = FALSE;
    
    // Initialize config with defaults
    app->config.theme = OTTSR_THEME_LIGHT;
    app->config.sound_volume = 70;
    app->config.minimize_to_tray = TRUE;
    app->config.autostart_sessions = FALSE;
    app->config.window_width = OTTSR_WINDOW_WIDTH;
    app->config.window_height = OTTSR_WINDOW_HEIGHT;
    
    // Create default profiles
    strcpy(app->config.profiles[0].name, "Pomodoro");
    app->config.profiles[0].study_minutes = 25;
    app->config.profiles[0].break_minutes = 5;
    app->config.profiles[0].long_break_minutes = 15;
    app->config.profiles[0].sessions_until_long_break = 4;
    app->config.profiles[0].sound_enabled = TRUE;
    app->config.profiles[0].notifications_enabled = TRUE;
    app->config.profiles[0].total_study_time = 0;
    app->config.profiles[0].total_sessions = 0;
    app->config.profiles[0].completed_sessions = 0;
    
    strcpy(app->config.profiles[1].name, "Deep Work");
    app->config.profiles[1].study_minutes = 90;
    app->config.profiles[1].break_minutes = 20;
    app->config.profiles[1].long_break_minutes = 30;
    app->config.profiles[1].sessions_until_long_break = 2;
    app->config.profiles[1].sound_enabled = TRUE;
    app->config.profiles[1].notifications_enabled = TRUE;
    app->config.profiles[1].total_study_time = 0;
    app->config.profiles[1].total_sessions = 0;
    app->config.profiles[1].completed_sessions = 0;
    
    strcpy(app->config.profiles[2].name, "Short Sprint");
    app->config.profiles[2].study_minutes = 15;
    app->config.profiles[2].break_minutes = 3;
    app->config.profiles[2].long_break_minutes = 10;
    app->config.profiles[2].sessions_until_long_break = 3;
    app->config.profiles[2].sound_enabled = TRUE;
    app->config.profiles[2].notifications_enabled = TRUE;
    app->config.profiles[2].total_study_time = 0;
    app->config.profiles[2].total_sessions = 0;
    app->config.profiles[2].completed_sessions = 0;
    
    app->config.profile_count = 3;
    app->config.active_profile = 0;
    strcpy(app->config.last_subject, "");
    
    // Load saved config
    if (!ottsr_load_config(app)) {
        g_print("Using default configuration\n");
    }
}

// Get configuration directory path
char* ottsr_get_config_path(void) {
    const char* home = g_get_home_dir();
    if (!home) return NULL;
    
    return g_build_filename(home, OTTSR_CONFIG_DIR, NULL);
}

// Load configuration from JSON file
gboolean ottsr_load_config(ottsr_app_t *app) {
    char *config_dir = ottsr_get_config_path();
    if (!config_dir) return FALSE;
    
    char *config_file = g_build_filename(config_dir, OTTSR_CONFIG_FILE, NULL);
    
    GError *error = NULL;
    JsonParser *parser = json_parser_new();
    
    if (!json_parser_load_from_file(parser, config_file, &error)) {
        g_free(config_dir);
        g_free(config_file);
        g_object_unref(parser);
        if (error) g_error_free(error);
        return FALSE;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_free(config_dir);
        g_free(config_file);
        g_object_unref(parser);
        return FALSE;
    }
    
    JsonObject *root_obj = json_node_get_object(root);
    
    // Load basic settings
    if (json_object_has_member(root_obj, "active_profile")) {
        app->config.active_profile = json_object_get_int_member(root_obj, "active_profile");
    }
    
    if (json_object_has_member(root_obj, "theme")) {
        app->config.theme = json_object_get_int_member(root_obj, "theme");
    }
    
    if (json_object_has_member(root_obj, "sound_volume")) {
        app->config.sound_volume = json_object_get_int_member(root_obj, "sound_volume");
    }
    
    if (json_object_has_member(root_obj, "last_subject")) {
        const char* subject = json_object_get_string_member(root_obj, "last_subject");
        if (subject) {
            strncpy(app->config.last_subject, subject, OTTSR_MAX_NAME_LEN - 1);
            app->config.last_subject[OTTSR_MAX_NAME_LEN - 1] = '\0';
        }
    }
    
    // Load profiles
    if (json_object_has_member(root_obj, "profiles")) {
        JsonArray *profiles_array = json_object_get_array_member(root_obj, "profiles");
        guint profile_count = json_array_get_length(profiles_array);
        
        if (profile_count > OTTSR_MAX_PROFILES) {
            profile_count = OTTSR_MAX_PROFILES;
        }
        
        for (guint i = 0; i < profile_count; i++) {
            JsonObject *profile_obj = json_array_get_object_element(profiles_array, i);
            ottsr_profile_t *profile = &app->config.profiles[i];
            
            const char *name = json_object_get_string_member(profile_obj, "name");
            if (name) {
                strncpy(profile->name, name, OTTSR_MAX_NAME_LEN - 1);
                profile->name[OTTSR_MAX_NAME_LEN - 1] = '\0';
            }
            
            profile->study_minutes = json_object_get_int_member(profile_obj, "study_minutes");
            profile->break_minutes = json_object_get_int_member(profile_obj, "break_minutes");
            profile->long_break_minutes = json_object_get_int_member(profile_obj, "long_break_minutes");
            profile->sessions_until_long_break = json_object_get_int_member(profile_obj, "sessions_until_long_break");
            profile->sound_enabled = json_object_get_boolean_member(profile_obj, "sound_enabled");
            profile->notifications_enabled = json_object_get_boolean_member(profile_obj, "notifications_enabled");
            profile->total_study_time = json_object_get_int_member(profile_obj, "total_study_time");
            profile->total_sessions = json_object_get_int_member(profile_obj, "total_sessions");
            profile->completed_sessions = json_object_get_int_member(profile_obj, "completed_sessions");
        }
        
        app->config.profile_count = profile_count;
    }
    
    g_free(config_dir);
    g_free(config_file);
    g_object_unref(parser);
    return TRUE;
}

// Save configuration to JSON file
gboolean ottsr_save_config(ottsr_app_t *app) {
    char *config_dir = ottsr_get_config_path();
    if (!config_dir) return FALSE;
    
    // Create config directory if it doesn't exist
    g_mkdir_with_parents(config_dir, 0755);
    
    char *config_file = g_build_filename(config_dir, OTTSR_CONFIG_FILE, NULL);
    
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    
    // Save basic settings
    json_builder_set_member_name(builder, "active_profile");
    json_builder_add_int_value(builder, app->config.active_profile);
    
    json_builder_set_member_name(builder, "theme");
    json_builder_add_int_value(builder, app->config.theme);
    
    json_builder_set_member_name(builder, "sound_volume");
    json_builder_add_int_value(builder, app->config.sound_volume);
    
    json_builder_set_member_name(builder, "last_subject");
    json_builder_add_string_value(builder, app->config.last_subject);
    
    // Save profiles
    json_builder_set_member_name(builder, "profiles");
    json_builder_begin_array(builder);
    
    for (int i = 0; i < app->config.profile_count; i++) {
        ottsr_profile_t *profile = &app->config.profiles[i];
        
        json_builder_begin_object(builder);
        
        json_builder_set_member_name(builder, "name");
        json_builder_add_string_value(builder, profile->name);
        
        json_builder_set_member_name(builder, "study_minutes");
        json_builder_add_int_value(builder, profile->study_minutes);
        
        json_builder_set_member_name(builder, "break_minutes");
        json_builder_add_int_value(builder, profile->break_minutes);
        
        json_builder_set_member_name(builder, "long_break_minutes");
        json_builder_add_int_value(builder, profile->long_break_minutes);
        
        json_builder_set_member_name(builder, "sessions_until_long_break");
        json_builder_add_int_value(builder, profile->sessions_until_long_break);
        
        json_builder_set_member_name(builder, "sound_enabled");
        json_builder_add_boolean_value(builder, profile->sound_enabled);
        
        json_builder_set_member_name(builder, "notifications_enabled");
        json_builder_add_boolean_value(builder, profile->notifications_enabled);
        
        json_builder_set_member_name(builder, "total_study_time");
        json_builder_add_int_value(builder, profile->total_study_time);
        
        json_builder_set_member_name(builder, "total_sessions");
        json_builder_add_int_value(builder, profile->total_sessions);
        
        json_builder_set_member_name(builder, "completed_sessions");
        json_builder_add_int_value(builder, profile->completed_sessions);
        
        json_builder_end_object(builder);
    }
    
    json_builder_end_array(builder);
    json_builder_end_object(builder);
    
    JsonGenerator *generator = json_generator_new();
    JsonNode *root = json_builder_get_root(builder);
    json_generator_set_root(generator, root);
    
    GError *error = NULL;
    gboolean success = json_generator_to_file(generator, config_file, &error);
    
    if (!success && error) {
        g_warning("Failed to save config: %s", error->message);
        g_error_free(error);
    }
    
    g_free(config_dir);
    g_free(config_file);
    g_object_unref(builder);
    g_object_unref(generator);
    json_node_free(root);
    
    return success;
}

// Format time display
void ottsr_format_time(int seconds, char *buffer, size_t buffer_size) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        snprintf(buffer, buffer_size, "%d:%02d:%02d", hours, minutes, secs);
    } else {
        snprintf(buffer, buffer_size, "%02d:%02d", minutes, secs);
    }
}

// Format statistics display
void ottsr_format_stats(ottsr_app_t *app, char *buffer, size_t buffer_size) {
    ottsr_profile_t *profile = &app->config.profiles[app->config.active_profile];
    
    int hours = profile->total_study_time / 3600;
    int minutes = (profile->total_study_time % 3600) / 60;
    
    snprintf(buffer, buffer_size, 
             "Sessions completed: %d | Total time: %dh %dm | Current session: %d",
             profile->completed_sessions, hours, minutes, app->session.current_sessions);
}

// Update display elements
void ottsr_update_display(ottsr_app_t *app) {
    if (!app->timer_label || !app->status_label || !app->stats_label) return;
    
    ottsr_profile_t *profile = &app->config.profiles[app->config.active_profile];
    
    // Update timer display
    char time_str[32];
    int remaining_time = 0;
    
    if (app->session.state == OTTSR_STATE_STUDYING) {
        remaining_time = profile->study_minutes * 60 - app->session.elapsed_study_seconds;
    } else if (app->session.state == OTTSR_STATE_BREAKING) {
        int break_duration = app->session.is_long_break ? 
                           profile->long_break_minutes : profile->break_minutes;
        remaining_time = break_duration * 60 - app->session.elapsed_break_seconds;
    } else {
        remaining_time = profile->study_minutes * 60;
    }
    
    if (remaining_time < 0) remaining_time = 0;
    ottsr_format_time(remaining_time, time_str, sizeof(time_str));
    gtk_label_set_text(GTK_LABEL(app->timer_label), time_str);
    
    // Update progress bars
    if (app->session_progress) {
        double progress = 0.0;
        if (app->session.state == OTTSR_STATE_STUDYING) {
            progress = (double)app->session.elapsed_study_seconds / (profile->study_minutes * 60);
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), progress);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->session_progress), 
                                 app->session.state == OTTSR_STATE_STUDYING ? "Studying..." : "");
    }
    
    if (app->break_progress) {
        double progress = 0.0;
        if (app->session.state == OTTSR_STATE_BREAKING) {
            int break_duration = app->session.is_long_break ? 
                               profile->long_break_minutes : profile->break_minutes;
            progress = (double)app->session.elapsed_break_seconds / (break_duration * 60);
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->break_progress), progress);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->break_progress), 
                                 app->session.state == OTTSR_STATE_BREAKING ? "On break..." : "");
    }
    
    // Update stats
    char stats_str[256];
    ottsr_format_stats(app, stats_str, sizeof(stats_str));
    gtk_label_set_text(GTK_LABEL(app->stats_label), stats_str);
    
    // Update time spinners
    if (app->study_time_spin && app->break_time_spin) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->study_time_spin), profile->study_minutes);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->break_time_spin), profile->break_minutes);
    }
}

// Timer callback for session management
gboolean ottsr_timer_callback(gpointer user_data) {
    ottsr_app_t *app = (ottsr_app_t *)user_data;
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    if (app->session.state == OTTSR_STATE_STUDYING) {
        app->session.elapsed_study_seconds++;
        
        // Check if study session is complete
        if (app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
            app->session.current_sessions++;
            profile->completed_sessions++;
            
            // Determine if this should be a long break
            app->session.is_long_break = 
                (app->session.current_sessions % profile->sessions_until_long_break == 0);
            
            // Switch to break
            app->session.state = OTTSR_STATE_BREAKING;
            app->session.break_start = time(NULL);
            app->session.elapsed_break_seconds = 0;
            
            const char *break_type = app->session.is_long_break ? "Long Break" : "Break";
            gtk_label_set_text(GTK_LABEL(app->status_label), break_type);
            
            ottsr_show_notification(app, "Study Session Complete!", 
                                  app->session.is_long_break ? "Time for a long break!" : "Time for a break!");
            ottsr_play_notification_sound(app);
        }
    } else if (app->session.state == OTTSR_STATE_BREAKING) {
        app->session.elapsed_break_seconds++;
        
        int break_duration = app->session.is_long_break ? 
                           profile->long_break_minutes : profile->break_minutes;
        
        // Check if break is complete
        if (app->session.elapsed_break_seconds >= break_duration * 60) {
            if (app->config.autostart_sessions) {
                // Automatically start next session
                app->session.state = OTTSR_STATE_STUDYING;
                app->session.session_start = time(NULL);
                app->session.elapsed_study_seconds = 0;
                gtk_label_set_text(GTK_LABEL(app->status_label), "Studying...");
                
                ottsr_show_notification(app, "Break Complete!", "Back to studying!");
                ottsr_play_notification_sound(app);
            } else {
                // Stop and wait for user to start next session
                ottsr_stop_session(app);
                ottsr_show_notification(app, "Break Complete!", "Ready for your next study session!");
                ottsr_play_notification_sound(app);
            }
        }
    }
    
    return G_SOURCE_CONTINUE;
}

// UI update callback
gboolean ottsr_ui_update_callback(gpointer user_data) {
    ottsr_app_t *app = (ottsr_app_t *)user_data;
    ottsr_update_display(app);
    return G_SOURCE_CONTINUE;
}

// Show system notification
void ottsr_show_notification(ottsr_app_t *app, const char *title, const char *message) {
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    if (!profile->notifications_enabled) return;
    
    GNotification *notification = g_notification_new(title);
    g_notification_set_body(notification, message);
    g_notification_set_priority(notification, G_NOTIFICATION_PRIORITY_NORMAL);
    
    g_application_send_notification(G_APPLICATION(app->app), NULL, notification);
    g_object_unref(notification);
}

// Play notification sound
void ottsr_play_notification_sound(ottsr_app_t *app) {
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    if (!profile->sound_enabled) return;
    
    // Simple beep - on Windows this should work
    g_print("\a");
}

// Create main window with modern design
void ottsr_create_main_window(ottsr_app_t *app) {
    GError *error = NULL;
    
    // Create main window
    app->main_window = gtk_application_window_new(app->app);
    if (!app->main_window) {
        g_warning("Failed to create main window");
        return;
    }
    
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
    app->session.pause_duration = 0;
    
    const char *subject = gtk_entry_get_text(GTK_ENTRY(app->subject_entry));
    strncpy(app->session.current_subject, subject, OTTSR_MAX_NAME_LEN - 1);
    app->session.current_subject[OTTSR_MAX_NAME_LEN - 1] = '\0';
    
    // Save subject for next time
    strncpy(app->config.last_subject, subject, OTTSR_MAX_NAME_LEN - 1);
    app->config.last_subject[OTTSR_MAX_NAME_LEN - 1] = '\0';
    
    app->config.profiles[profile_idx].total_sessions++;
    
    // Start timers
    app->session_timer_id = g_timeout_add_seconds(1, ottsr_timer_callback, app);
    app->ui_update_timer_id = g_timeout_add(100, ottsr_ui_update_callback, app);
    
    // Update UI
    gtk_widget_set_sensitive(app->start_button, FALSE);
    gtk_widget_set_sensitive(app->pause_button, TRUE);
    gtk_widget_set_sensitive(app->stop_button, TRUE);
    gtk_widget_set_sensitive(app->profile_combo, FALSE);
    gtk_widget_set_sensitive(app->study_time_spin, FALSE);
    gtk_widget_set_sensitive(app->break_time_spin, FALSE);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Studying...");
    
    ottsr_update_display(app);
}

void ottsr_pause_session(ottsr_app_t *app) {
    if (app->session.state != OTTSR_STATE_STUDYING && 
        app->session.state != OTTSR_STATE_BREAKING) return;
    
    if (app->session.state == OTTSR_STATE_PAUSED) {
        // Resume session
        app->session.pause_duration += time(NULL) - app->session.pause_start;
        
        if (app->session.elapsed_study_seconds > 0) {
            app->session.state = OTTSR_STATE_STUDYING;
            gtk_label_set_text(GTK_LABEL(app->status_label), "Studying...");
        } else {
            app->session.state = OTTSR_STATE_BREAKING;
            const char *break_type = app->session.is_long_break ? "Long Break" : "Break";
            gtk_label_set_text(GTK_LABEL(app->status_label), break_type);
        }
        
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
        
        // Resume timers
        app->session_timer_id = g_timeout_add_seconds(1, ottsr_timer_callback, app);
        app->ui_update_timer_id = g_timeout_add(100, ottsr_ui_update_callback, app);
    } else {
        // Pause session
        app->session.state = OTTSR_STATE_PAUSED;
        app->session.pause_start = time(NULL);
        gtk_label_set_text(GTK_LABEL(app->status_label), "Paused");
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Resume");
        
        // Stop timers
        if (app->session_timer_id > 0) {
            g_source_remove(app->session_timer_id);
            app->session_timer_id = 0;
        }
        if (app->ui_update_timer_id > 0) {
            g_source_remove(app->ui_update_timer_id);
            app->ui_update_timer_id = 0;
        }
    }
    
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
    
    // Reset state
    app->session.state = OTTSR_STATE_IDLE;
    app->session.current_sessions = 0;
    app->session.pause_duration = 0;
    
    // Update UI
    gtk_widget_set_sensitive(app->start_button, TRUE);
    gtk_widget_set_sensitive(app->pause_button, FALSE);
    gtk_widget_set_sensitive(app->stop_button, FALSE);
    gtk_widget_set_sensitive(app->profile_combo, TRUE);
    gtk_widget_set_sensitive(app->study_time_spin, TRUE);
    gtk_widget_set_sensitive(app->break_time_spin, TRUE);
    gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
    gtk_label_set_text(GTK_LABEL(app->status_label), "Ready to start studying");
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), 0.0);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->break_progress), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->session_progress), "");
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->break_progress), "");
    
    ottsr_save_config(app);
    ottsr_update_display(app);
}

// Settings window
void ottsr_create_settings_window(ottsr_app_t *app) {
    if (app->settings_window) {
        gtk_window_present(GTK_WINDOW(app->settings_window));
        return;
    }
    
    app->settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->settings_window), "Settings");
    gtk_window_set_default_size(GTK_WINDOW(app->settings_window), 400, 300);
    gtk_window_set_transient_for(GTK_WINDOW(app->settings_window), 
                                GTK_WINDOW(app->main_window));
    gtk_window_set_modal(GTK_WINDOW(app->settings_window), TRUE);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 20);
    gtk_container_add(GTK_CONTAINER(app->settings_window), main_box);
    
    // Theme selection
    GtkWidget *theme_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), theme_box, FALSE, FALSE, 0);
    
    GtkWidget *theme_label = gtk_label_new("Theme:");
    gtk_widget_set_size_request(theme_label, 120, -1);
    gtk_widget_set_halign(theme_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(theme_box), theme_label, FALSE, FALSE, 0);
    
    app->theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->theme_combo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->theme_combo), "Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->theme_combo), "Auto");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->theme_combo), app->config.theme);
    gtk_box_pack_start(GTK_BOX(theme_box), app->theme_combo, TRUE, TRUE, 0);
    
    // Sound settings
    GtkWidget *sound_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), sound_box, FALSE, FALSE, 0);
    
    app->sound_check = gtk_check_button_new_with_label("Enable notification sounds");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->sound_check), 
                                app->config.profiles[app->config.active_profile].sound_enabled);
    gtk_box_pack_start(GTK_BOX(sound_box), app->sound_check, FALSE, FALSE, 0);
    
    // Volume setting
    GtkWidget *volume_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), volume_box, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Volume:");
    gtk_widget_set_size_request(volume_label, 120, -1);
    gtk_widget_set_halign(volume_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(volume_box), volume_label, FALSE, FALSE, 0);
    
    app->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(app->volume_scale), app->config.sound_volume);
    gtk_box_pack_start(GTK_BOX(volume_box), app->volume_scale, TRUE, TRUE, 0);
    
    // Notification settings
    app->notifications_check = gtk_check_button_new_with_label("Enable desktop notifications");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->notifications_check), 
                                app->config.profiles[app->config.active_profile].notifications_enabled);
    gtk_box_pack_start(GTK_BOX(main_box), app->notifications_check, FALSE, FALSE, 0);
    
    // Auto-start sessions
    app->autostart_check = gtk_check_button_new_with_label("Auto-start sessions after breaks");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->autostart_check), 
                                app->config.autostart_sessions);
    gtk_box_pack_start(GTK_BOX(main_box), app->autostart_check, FALSE, FALSE, 0);
    
    // Minimize to tray
    app->minimize_check = gtk_check_button_new_with_label("Minimize to system tray");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->minimize_check), 
                                app->config.minimize_to_tray);
    gtk_box_pack_start(GTK_BOX(main_box), app->minimize_check, FALSE, FALSE, 0);
    
    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(button_box, 20);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_settings_cancel_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, FALSE, FALSE, 0);
    
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_style_context_add_class(gtk_widget_get_style_context(save_btn), "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_settings_save_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), save_btn, FALSE, FALSE, 0);
    
    // Show window
    gtk_widget_show_all(app->settings_window);
}

// Profile management window
void ottsr_create_profiles_window(ottsr_app_t *app) {
    if (app->profiles_window) {
        gtk_window_present(GTK_WINDOW(app->profiles_window));
        return;
    }
    
    app->profiles_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->profiles_window), "Manage Profiles");
    gtk_window_set_default_size(GTK_WINDOW(app->profiles_window), 600, 400);
    gtk_window_set_transient_for(GTK_WINDOW(app->profiles_window), 
                                GTK_WINDOW(app->main_window));
    gtk_window_set_modal(GTK_WINDOW(app->profiles_window), TRUE);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 20);
    gtk_container_add(GTK_CONTAINER(app->profiles_window), main_box);
    
    // Profile list
    GtkWidget *list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(list_box, 200, -1);
    gtk_box_pack_start(GTK_BOX(main_box), list_box, FALSE, FALSE, 0);
    
    GtkWidget *list_label = gtk_label_new("Profiles:");
    gtk_widget_set_halign(list_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(list_box), list_label, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 200);
    gtk_box_pack_start(GTK_BOX(list_box), scrolled, TRUE, TRUE, 0);
    
    app->profile_list = gtk_list_box_new();
    g_signal_connect(app->profile_list, "row-selected", G_CALLBACK(on_profile_list_changed), app);
    gtk_container_add(GTK_CONTAINER(scrolled), app->profile_list);
    
    // Populate profile list
    for (int i = 0; i < app->config.profile_count; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(app->config.profiles[i].name);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(app->profile_list), row, -1);
    }
    
    // Profile management buttons
    GtkWidget *profile_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(list_box), profile_buttons, FALSE, FALSE, 0);
    
    GtkWidget *add_btn = gtk_button_new_with_label("Add");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_profile_add_clicked), app);
    gtk_box_pack_start(GTK_BOX(profile_buttons), add_btn, TRUE, TRUE, 0);
    
    GtkWidget *delete_btn = gtk_button_new_with_label("Delete");
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_profile_delete_clicked), app);
    gtk_box_pack_start(GTK_BOX(profile_buttons), delete_btn, TRUE, TRUE, 0);
    
    // Profile editor
    GtkWidget *editor_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_box_pack_start(GTK_BOX(main_box), editor_box, TRUE, TRUE, 0);
    
    GtkWidget *editor_label = gtk_label_new("Profile Settings:");
    gtk_widget_set_halign(editor_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(editor_box), editor_label, FALSE, FALSE, 0);
    
    // Profile name
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(editor_box), name_box, FALSE, FALSE, 0);
    
    GtkWidget *name_label = gtk_label_new("Name:");
    gtk_widget_set_size_request(name_label, 120, -1);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(name_box), name_label, FALSE, FALSE, 0);
    
    app->profile_name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(name_box), app->profile_name_entry, TRUE, TRUE, 0);
    
    // Study time
    GtkWidget *study_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(editor_box), study_box, FALSE, FALSE, 0);
    
    GtkWidget *study_label = gtk_label_new("Study time (min):");
    gtk_widget_set_size_request(study_label, 120, -1);
    gtk_widget_set_halign(study_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(study_box), study_label, FALSE, FALSE, 0);
    
    app->profile_study_spin = gtk_spin_button_new_with_range(1, 180, 1);
    gtk_box_pack_start(GTK_BOX(study_box), app->profile_study_spin, TRUE, TRUE, 0);
    
    // Break time
    GtkWidget *break_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(editor_box), break_box, FALSE, FALSE, 0);
    
    GtkWidget *break_label = gtk_label_new("Break time (min):");
    gtk_widget_set_size_request(break_label, 120, -1);
    gtk_widget_set_halign(break_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(break_box), break_label, FALSE, FALSE, 0);
    
    app->profile_break_spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_box_pack_start(GTK_BOX(break_box), app->profile_break_spin, TRUE, TRUE, 0);
    
    // Long break time
    GtkWidget *longbreak_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(editor_box), longbreak_box, FALSE, FALSE, 0);
    
    GtkWidget *longbreak_label = gtk_label_new("Long break (min):");
    gtk_widget_set_size_request(longbreak_label, 120, -1);
    gtk_widget_set_halign(longbreak_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(longbreak_box), longbreak_label, FALSE, FALSE, 0);
    
    app->profile_longbreak_spin = gtk_spin_button_new_with_range(5, 120, 1);
    gtk_box_pack_start(GTK_BOX(longbreak_box), app->profile_longbreak_spin, TRUE, TRUE, 0);
    
    // Sessions until long break
    GtkWidget *sessions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(editor_box), sessions_box, FALSE, FALSE, 0);
    
    GtkWidget *sessions_label = gtk_label_new("Sessions until long break:");
    gtk_widget_set_size_request(sessions_label, 120, -1);
    gtk_widget_set_halign(sessions_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sessions_box), sessions_label, FALSE, FALSE, 0);
    
    app->profile_sessions_spin = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_box_pack_start(GTK_BOX(sessions_box), app->profile_sessions_spin, TRUE, TRUE, 0);
    
    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(button_box, 20);
    gtk_box_pack_start(GTK_BOX(editor_box), button_box, FALSE, FALSE, 0);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Close");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_profile_cancel_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, FALSE, FALSE, 0);
    
    GtkWidget *save_btn = gtk_button_new_with_label("Save Profile");
    gtk_style_context_add_class(gtk_widget_get_style_context(save_btn), "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_profile_save_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), save_btn, FALSE, FALSE, 0);
    
    // Select first profile if available
    if (app->config.profile_count > 0) {
        GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->profile_list), 0);
        if (first_row) {
            gtk_list_box_select_row(GTK_LIST_BOX(app->profile_list), first_row);
        }
    }
    
    gtk_widget_show_all(app->profiles_window);
}

// Cleanup function
void ottsr_cleanup_app(ottsr_app_t *app) {
    // Stop any running timers
    if (app->session_timer_id > 0) {
        g_source_remove(app->session_timer_id);
        app->session_timer_id = 0;
    }
    if (app->ui_update_timer_id > 0) {
        g_source_remove(app->ui_update_timer_id);
        app->ui_update_timer_id = 0;
    }
    
    // Save configuration
    ottsr_save_config(app);
    
    // Clean up CSS provider
    if (app->css_provider) {
        g_object_unref(app->css_provider);
        app->css_provider = NULL;
    }
}

// Callback implementations
void on_profile_changed(GtkComboBox *combo, ottsr_app_t *app) {
    int new_profile = gtk_combo_box_get_active(combo);
    if (new_profile >= 0 && new_profile < app->config.profile_count) {
        app->config.active_profile = new_profile;
        ottsr_update_display(app);
    }
}

void on_start_clicked(GtkButton *button, ottsr_app_t *app) {
    ottsr_start_session(app);
}

void on_pause_clicked(GtkButton *button, ottsr_app_t *app) {
    ottsr_pause_session(app);
}

void on_stop_clicked(GtkButton *button, ottsr_app_t *app) {
    ottsr_stop_session(app);
}

void on_time_changed(GtkSpinButton *spin, ottsr_app_t *app) {
    if (app->session.state != OTTSR_STATE_IDLE) return;
    
    ottsr_profile_t *profile = &app->config.profiles[app->config.active_profile];
    
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

void on_settings_clicked(GtkButton *button, ottsr_app_t *app) {
    ottsr_create_settings_window(app);
}

void on_profiles_clicked(GtkButton *button, ottsr_app_t *app) {
    ottsr_create_profiles_window(app);
}

void on_about_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Study Timer Pro");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), OTTSR_VERSION);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), 
                                 "A modern Pomodoro timer for enhanced productivity");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://github.com/g-flame/ottsr");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2024");
    
    const char *authors[] = {"Developer", NULL};
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->main_window));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Settings callbacks
void on_settings_save_clicked(GtkButton *button, ottsr_app_t *app) {
    // Save settings
    app->config.theme = gtk_combo_box_get_active(GTK_COMBO_BOX(app->theme_combo));
    app->config.sound_volume = gtk_range_get_value(GTK_RANGE(app->volume_scale));
    app->config.autostart_sessions = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->autostart_check));
    app->config.minimize_to_tray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->minimize_check));
    
    // Update current profile settings
    ottsr_profile_t *profile = &app->config.profiles[app->config.active_profile];
    profile->sound_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->sound_check));
    profile->notifications_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->notifications_check));
    
    ottsr_save_config(app);
    gtk_widget_destroy(app->settings_window);
    app->settings_window = NULL;
}

void on_settings_cancel_clicked(GtkButton *button, ottsr_app_t *app) {
    gtk_widget_destroy(app->settings_window);
    app->settings_window = NULL;
}

// Profile callbacks
void on_profile_add_clicked(GtkButton *button, ottsr_app_t *app) {
    if (app->config.profile_count >= OTTSR_MAX_PROFILES) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_OK,
                                                  "Maximum number of profiles reached (%d)",
                                                  OTTSR_MAX_PROFILES);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Create new profile with defaults
    int new_index = app->config.profile_count;
    ottsr_profile_t *new_profile = &app->config.profiles[new_index];
    
    snprintf(new_profile->name, OTTSR_MAX_NAME_LEN, "Profile %d", new_index + 1);
    new_profile->study_minutes = 25;
    new_profile->break_minutes = 5;
    new_profile->long_break_minutes = 15;
    new_profile->sessions_until_long_break = 4;
    new_profile->sound_enabled = TRUE;
    new_profile->notifications_enabled = TRUE;
    new_profile->total_study_time = 0;
    new_profile->total_sessions = 0;
    new_profile->completed_sessions = 0;
    
    app->config.profile_count++;
    
    // Add to list
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(new_profile->name);
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(GTK_LIST_BOX(app->profile_list), row, -1);
    gtk_widget_show_all(row);
    
    // Select the new profile
    gtk_list_box_select_row(GTK_LIST_BOX(app->profile_list), GTK_LIST_BOX_ROW(row));
    
    // Update main window combo box
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->profile_combo), new_profile->name);
}

void on_profile_delete_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->profile_list));
    if (!selected) return;
    
    if (app->config.profile_count <= 1) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_OK,
                                                  "Cannot delete the last profile.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    int index = gtk_list_box_row_get_index(selected);
    
    // Confirm deletion
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Delete profile '%s'?",
                                              app->config.profiles[index].name);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response != GTK_RESPONSE_YES) return;
    
    // Remove from array
    for (int i = index; i < app->config.profile_count - 1; i++) {
        app->config.profiles[i] = app->config.profiles[i + 1];
    }
    app->config.profile_count--;
    
    // Adjust active profile if needed
    if (app->config.active_profile >= index) {
        app->config.active_profile = 0;
    }
    
    // Remove from list
    gtk_widget_destroy(GTK_WIDGET(selected));
    
    // Rebuild main window combo box
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(app->profile_combo));
    for (int i = 0; i < app->config.profile_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->profile_combo), 
                                      app->config.profiles[i].name);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->profile_combo), app->config.active_profile);
    
    ottsr_update_display(app);
}

void on_profile_save_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->profile_list));
    if (!selected) return;
    
    int index = gtk_list_box_row_get_index(selected);
    ottsr_profile_t *profile = &app->config.profiles[index];
    
    // Get values from widgets
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->profile_name_entry));
    strncpy(profile->name, name, OTTSR_MAX_NAME_LEN - 1);
    profile->name[OTTSR_MAX_NAME_LEN - 1] = '\0';
    
    profile->study_minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_study_spin));
    profile->break_minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_break_spin));
    profile->long_break_minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_longbreak_spin));
    profile->sessions_until_long_break = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_sessions_spin));
    
    // Update list display
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(selected));
    if (GTK_IS_LABEL(child)) {
        gtk_label_set_text(GTK_LABEL(child), profile->name);
    }
    
    // Update main window combo box
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(app->profile_combo));
    for (int i = 0; i < app->config.profile_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->profile_combo), 
                                      app->config.profiles[i].name);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->profile_combo), app->config.active_profile);
    
    ottsr_save_config(app);
    ottsr_update_display(app);
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "Profile saved successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_profile_cancel_clicked(GtkButton *button, ottsr_app_t *app) {
    gtk_widget_destroy(app->profiles_window);
    app->profiles_window = NULL;
}

void on_profile_list_changed(GtkListBox *list, GtkListBoxRow *row, ottsr_app_t *app) {
    if (!row) return;
    
    int index = gtk_list_box_row_get_index(row);
    if (index < 0 || index >= app->config.profile_count) return;
    
    ottsr_profile_t *profile = &app->config.profiles[index];
    
    // Update editor widgets
    gtk_entry_set_text(GTK_ENTRY(app->profile_name_entry), profile->name);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->profile_study_spin), profile->study_minutes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->profile_break_spin), profile->break_minutes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->profile_longbreak_spin), profile->long_break_minutes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->profile_sessions_spin), profile->sessions_until_long_break);
}

// Main function
int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("com.github.g-flame.ottsr", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(ottsr_activate), &g_app);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
    ottsr_cleanup_app(&g_app);
    g_object_unref(app);
    
    return status;
}
