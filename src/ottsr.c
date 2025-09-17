// Utility functions
void ottsr_format_time(int seconds, char *buffer, size_t buffer_size) {
    int minutes = seconds / 60;
    seconds = seconds % 60;
    snprintf(buffer, buffer_size, "%02d:%02d", minutes, seconds);
}

void ottsr_format_stats(ottsr_app_t *app, char *buffer, size_t buffer_size) {
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    int total_hours = profile->total_study_time / 3600;
    int total_minutes = (profile->total_study_time % 3600) / 60;
    
    snprintf(buffer, buffer_size, 
             "Total Study Time: %dh %dm | Sessions: %d/%d completed",
             total_hours, total_minutes, 
             profile->completed_sessions, profile->total_sessions);
}

char* ottsr_get_config_path(void) {
    const char *home = g_get_home_dir();
    if (!home) return NULL;
    
    return g_build_filename(home, OTTSR_CONFIG_DIR, OTTSR_CONFIG_FILE, NULL);
}

void ottsr_show_notification(ottsr_app_t *app, const char *title, const char *message) {
    GNotification *notification = g_notification_new(title);
    g_notification_set_body(notification, message);
    g_notification_set_priority(notification, G_NOTIFICATION_PRIORITY_NORMAL);
    
    g_application_send_notification(G_APPLICATION(app->app), "timer", notification);
    g_object_unref(notification);
}

void ottsr_play_notification_sound(ottsr_app_t *app) {
    // Simple beep for now - could be enhanced with actual sound files
    gdk_beep();
}

void ottsr_cleanup_app(ottsr_app_t *app) {
    ottsr_save_config(app);
    
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

void on_settings_clicked(GtkButton *button, ottsr_app_t *app) {
    if (!app->settings_window) {
        ottsr_create_settings_window(app);
    }
    
    gtk_window_set_transient_for(GTK_WINDOW(app->settings_window), 
                                GTK_WINDOW(app->main_window));
    gtk_window_present(GTK_WINDOW(app->settings_window));
}

void on_profiles_clicked(GtkButton *button, ottsr_app_t *app) {
    if (!app->profiles_window) {
        ottsr_create_profiles_window(app);
    }
    
    gtk_window_set_transient_for(GTK_WINDOW(app->profiles_window), 
                                GTK_WINDOW(app->main_window));
    gtk_window_present(GTK_WINDOW(app->profiles_window));
}

void on_about_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkWidget *about_dialog = gtk_about_dialog_new();
    
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Study Timer Pro");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), OTTSR_VERSION);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), 
        "A modern, feature-rich study timer application with customizable profiles and statistics tracking.");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), "https://github.com/g-flame/ottsr");
    gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog), "Visit on GitHub");
    
    const char *authors[] = {"g-flame", "Rewritten with GTK by Claude", NULL};
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
    
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_MIT_X11);
    
    gtk_window_set_transient_for(GTK_WINDOW(about_dialog), GTK_WINDOW(app->main_window));
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
}

// Settings window
void ottsr_create_settings_window(ottsr_app_t *app) {
    app->settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->settings_window), "Settings");
    gtk_window_set_default_size(GTK_WINDOW(app->profiles_window), 600, 500);
    gtk_window_set_modal(GTK_WINDOW(app->profiles_window), TRUE);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 20);
    gtk_container_add(GTK_CONTAINER(app->profiles_window), main_box);
    
    // Left side - Profile list
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(left_box, 200, -1);
    gtk_box_pack_start(GTK_BOX(main_box), left_box, FALSE, FALSE, 0);
    
    GtkWidget *list_label = gtk_label_new("Study Profiles");
    gtk_widget_set_halign(list_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(left_box), list_label, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 300);
    gtk_box_pack_start(GTK_BOX(left_box), scrolled, TRUE, TRUE, 0);
    
    app->profile_list = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), app->profile_list);
    g_signal_connect(app->profile_list, "row-selected", G_CALLBACK(on_profile_list_changed), app);
    
    // Populate profile list
    for (int i = 0; i < app->config.profile_count; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(app->config.profiles[i].name);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(app->profile_list), row, -1);
    }
    
    // List buttons
    GtkWidget *list_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(left_box), list_button_box, FALSE, FALSE, 0);
    
    GtkWidget *add_btn = gtk_button_new_with_label("Add");
    gtk_style_context_add_class(gtk_widget_get_style_context(add_btn), "suggested-action");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_profile_add_clicked), app);
    gtk_box_pack_start(GTK_BOX(list_button_box), add_btn, TRUE, TRUE, 0);
    
    GtkWidget *delete_btn = gtk_button_new_with_label("Delete");
    gtk_style_context_add_class(gtk_widget_get_style_context(delete_btn), "destructive-action");
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_profile_delete_clicked), app);
    gtk_box_pack_start(GTK_BOX(list_button_box), delete_btn, TRUE, TRUE, 0);
    
    // Right side - Profile editor
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_box_pack_start(GTK_BOX(main_box), right_box, TRUE, TRUE, 0);
    
    GtkWidget *editor_label = gtk_label_new("Profile Settings");
    gtk_widget_set_halign(editor_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(right_box), editor_label, FALSE, FALSE, 0);
    
    // Profile name
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(right_box), name_box, FALSE, FALSE, 0);
    
    GtkWidget *name_label = gtk_label_new("Name:");
    gtk_widget_set_size_request(name_label, 120, -1);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(name_box), name_label, FALSE, FALSE, 0);
    
    app->profile_name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(name_box), app->profile_name_entry, TRUE, TRUE, 0);
    
    // Study minutes
    GtkWidget *study_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(right_box), study_box, FALSE, FALSE, 0);
    
    GtkWidget *study_label = gtk_label_new("Study Minutes:");
    gtk_widget_set_size_request(study_label, 120, -1);
    gtk_widget_set_halign(study_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(study_box), study_label, FALSE, FALSE, 0);
    
    app->profile_study_spin = gtk_spin_button_new_with_range(1, 180, 1);
    gtk_box_pack_start(GTK_BOX(study_box), app->profile_study_spin, FALSE, FALSE, 0);
    
    // Break minutes
    GtkWidget *break_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(right_box), break_box, FALSE, FALSE, 0);
    
    GtkWidget *break_label = gtk_label_new("Break Minutes:");
    gtk_widget_set_size_request(break_label, 120, -1);
    gtk_widget_set_halign(break_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(break_box), break_label, FALSE, FALSE, 0);
    
    app->profile_break_spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_box_pack_start(GTK_BOX(break_box), app->profile_break_spin, FALSE, FALSE, 0);
    
    // Long break minutes
    GtkWidget *longbreak_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(right_box), longbreak_box, FALSE, FALSE, 0);
    
    GtkWidget *longbreak_label = gtk_label_new("Long Break:");
    gtk_widget_set_size_request(longbreak_label, 120, -1);
    gtk_widget_set_halign(longbreak_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(longbreak_box), longbreak_label, FALSE, FALSE, 0);
    
    app->profile_longbreak_spin = gtk_spin_button_new_with_range(5, 120, 1);
    gtk_box_pack_start(GTK_BOX(longbreak_box), app->profile_longbreak_spin, FALSE, FALSE, 0);
    
    // Sessions until long break
    GtkWidget *sessions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(right_box), sessions_box, FALSE, FALSE, 0);
    
    GtkWidget *sessions_label = gtk_label_new("Sessions to Long Break:");
    gtk_widget_set_size_request(sessions_label, 120, -1);
    gtk_widget_set_halign(sessions_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(sessions_box), sessions_label, FALSE, FALSE, 0);
    
    app->profile_sessions_spin = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_box_pack_start(GTK_BOX(sessions_box), app->profile_sessions_spin, FALSE, FALSE, 0);
    
    // Bottom buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(right_box), button_box, FALSE, FALSE, 0);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_profile_cancel_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, FALSE, FALSE, 0);
    
    GtkWidget *save_btn = gtk_button_new_with_label("Save Changes");
    gtk_style_context_add_class(gtk_widget_get_style_context(save_btn), "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_profile_save_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), save_btn, FALSE, FALSE, 0);
    
    // Select first profile by default
    if (app->config.profile_count > 0) {
        GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->profile_list), 0);
        gtk_list_box_select_row(GTK_LIST_BOX(app->profile_list), first_row);
    }
    
    gtk_widget_show_all(app->profiles_window);
}

// Settings callbacks
void on_settings_save_clicked(GtkButton *button, ottsr_app_t *app) {
    // Save settings
    app->config.theme = gtk_combo_box_get_active(GTK_COMBO_BOX(app->theme_combo));
    app->config.sound_volume = (int)gtk_range_get_value(GTK_RANGE(app->volume_scale));
    app->config.minimize_to_tray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->minimize_check));
    app->config.autostart_sessions = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->autostart_check));
    
    // Update active profile settings
    ottsr_profile_t *profile = &app->config.profiles[app->config.active_profile];
    profile->sound_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->sound_check));
    profile->notifications_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->notifications_check));
    
    ottsr_save_config(app);
    gtk_widget_hide(app->settings_window);
}

void on_settings_cancel_clicked(GtkButton *button, ottsr_app_t *app) {
    gtk_widget_hide(app->settings_window);
}

// Profile callbacks
void on_profile_add_clicked(GtkButton *button, ottsr_app_t *app) {
    if (app->config.profile_count >= OTTSR_MAX_PROFILES) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Maximum number of profiles (%d) reached.", OTTSR_MAX_PROFILES);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Add new profile with defaults
    ottsr_profile_t *new_profile = &app->config.profiles[app->config.profile_count];
    snprintf(new_profile->name, OTTSR_MAX_NAME_LEN, "Profile %d", app->config.profile_count + 1);
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
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(GTK_LIST_BOX(app->profile_list), row, -1);
    gtk_widget_show_all(row);
    
    // Select the new profile
    gtk_list_box_select_row(GTK_LIST_BOX(app->profile_list), GTK_LIST_BOX_ROW(row));
}

void on_profile_delete_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->profile_list));
    if (!selected) return;
    
    if (app->config.profile_count <= 1) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Cannot delete the last profile.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    int index = gtk_list_box_row_get_index(selected);
    
    // Confirm deletion
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->profiles_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to delete profile '%s'?", 
        app->config.profiles[index].name);
        
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response != GTK_RESPONSE_YES) return;
    
    // Remove from array
    for (int i = index; i < app->config.profile_count - 1; i++) {
        app->config.profiles[i] = app->config.profiles[i + 1];
    }
    app->config.profile_count--;
    
    // Update active profile index if needed
    if (app->config.active_profile >= app->config.profile_count) {
        app->config.active_profile = app->config.profile_count - 1;
    }
    
    // Remove from list and select next
    gtk_container_remove(GTK_CONTAINER(app->profile_list), GTK_WIDGET(selected));
    
    // Select first available profile
    if (app->config.profile_count > 0) {
        GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->profile_list), 0);
        gtk_list_box_select_row(GTK_LIST_BOX(app->profile_list), first_row);
    }
}

void on_profile_save_clicked(GtkButton *button, ottsr_app_t *app) {
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->profile_list));
    if (!selected) return;
    
    int index = gtk_list_box_row_get_index(selected);
    ottsr_profile_t *profile = &app->config.profiles[index];
    
    // Save profile data
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->profile_name_entry));
    strncpy(profile->name, name, OTTSR_MAX_NAME_LEN - 1);
    profile->name[OTTSR_MAX_NAME_LEN - 1] = '\0';
    
    profile->study_minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_study_spin));
    profile->break_minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_break_spin));
    profile->long_break_minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_longbreak_spin));
    profile->sessions_until_long_break = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->profile_sessions_spin));
    
    // Update list item
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(selected));
    gtk_label_set_text(GTK_LABEL(label), profile->name);
    
    // Update main window combo if this profile is active
    if (index == app->config.active_profile) {
        gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(app->profile_combo), index);
        gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(app->profile_combo), index, profile->name);
        gtk_combo_box_set_active(GTK_COMBO_BOX(app->profile_combo), index);
        ottsr_update_display(app);
    }
    
    ottsr_save_config(app);
    gtk_widget_hide(app->profiles_window);
}

void on_profile_cancel_clicked(GtkButton *button, ottsr_app_t *app) {
    gtk_widget_hide(app->profiles_window);
}

void on_profile_list_changed(GtkListBox *list, GtkListBoxRow *row, ottsr_app_t *app) {
    if (!row) return;
    
    int index = gtk_list_box_row_get_index(row);
    if (index < 0 || index >= app->config.profile_count) return;
    
    ottsr_profile_t *profile = &app->config.profiles[index];
    
    // Update editor fields
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
}WINDOW(app->settings_window), 400, 500);
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
    
    // Volume control
    GtkWidget *volume_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), volume_box, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Sound Volume:");
    gtk_widget_set_size_request(volume_label, 120, -1);
    gtk_widget_set_halign(volume_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(volume_box), volume_label, FALSE, FALSE, 0);
    
    app->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(app->volume_scale), app->config.sound_volume);
    gtk_scale_set_draw_value(GTK_SCALE(app->volume_scale), TRUE);
    gtk_box_pack_start(GTK_BOX(volume_box), app->volume_scale, TRUE, TRUE, 0);
    
    // Checkboxes
    app->sound_check = gtk_check_button_new_with_label("Enable notification sounds");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->sound_check), 
                                app->config.profiles[app->config.active_profile].sound_enabled);
    gtk_box_pack_start(GTK_BOX(main_box), app->sound_check, FALSE, FALSE, 0);
    
    app->notifications_check = gtk_check_button_new_with_label("Enable desktop notifications");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->notifications_check), 
                                app->config.profiles[app->config.active_profile].notifications_enabled);
    gtk_box_pack_start(GTK_BOX(main_box), app->notifications_check, FALSE, FALSE, 0);
    
    app->minimize_check = gtk_check_button_new_with_label("Minimize to system tray");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->minimize_check), app->config.minimize_to_tray);
    gtk_box_pack_start(GTK_BOX(main_box), app->minimize_check, FALSE, FALSE, 0);
    
    app->autostart_check = gtk_check_button_new_with_label("Auto-start next session after break");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->autostart_check), app->config.autostart_sessions);
    gtk_box_pack_start(GTK_BOX(main_box), app->autostart_check, FALSE, FALSE, 0);
    
    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_settings_cancel_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, FALSE, FALSE, 0);
    
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_style_context_add_class(gtk_widget_get_style_context(save_btn), "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_settings_save_clicked), app);
    gtk_box_pack_start(GTK_BOX(button_box), save_btn, FALSE, FALSE, 0);
    
    gtk_widget_show_all(app->settings_window);
}

// Profiles window
void ottsr_create_profiles_window(ottsr_app_t *app) {
    app->profiles_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->profiles_window), "Manage Profiles");
    gtk_window_set_default_size(GTK_#include "ottsr.h"

static ottsr_app_t g_app = {0};

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
    GtkBuilder *builder;
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

void ottsr_pause_session(ottsr_app_t *app) {
    if (app->session.state == OTTSR_STATE_IDLE) return;
    
    if (app->session.state == OTTSR_STATE_PAUSED) {
        // Resume session
        time_t pause_duration = time(NULL) - app->session.pause_start;
        app->session.session_start += pause_duration;
        if (app->session.break_start > 0) {
            app->session.break_start += pause_duration;
        }
        
        if (app->session.break_start > 0) {
            app->session.state = OTTSR_STATE_BREAKING;
            gtk_label_set_text(GTK_LABEL(app->status_label), "Break time...");
        } else {
            app->session.state = OTTSR_STATE_STUDYING;
            gtk_label_set_text(GTK_LABEL(app->status_label), "Studying...");
        }
        
        app->session_timer_id = g_timeout_add_seconds(1, ottsr_timer_callback, app);
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Pause");
    } else {
        // Pause session
        if (app->session_timer_id > 0) {
            g_source_remove(app->session_timer_id);
            app->session_timer_id = 0;
        }
        
        app->session.pause_start = time(NULL);
        app->session.state = OTTSR_STATE_PAUSED;
        gtk_button_set_label(GTK_BUTTON(app->pause_button), "Resume");
        gtk_label_set_text(GTK_LABEL(app->status_label), "Paused");
    }
    
    ottsr_update_display(app);
}

// Update display with current state
void ottsr_update_display(ottsr_app_t *app) {
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    char time_str[32];
    int display_seconds = 0;
    double progress = 0.0;
    
    if (app->session.state == OTTSR_STATE_STUDYING || app->session.state == OTTSR_STATE_PAUSED) {
        display_seconds = (profile->study_minutes * 60) - app->session.elapsed_study_seconds;
        if (profile->study_minutes > 0) {
            progress = (double)app->session.elapsed_study_seconds / (profile->study_minutes * 60);
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), 
                                    CLAMP(progress, 0.0, 1.0));
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->break_progress), 0.0);
    } else if (app->session.state == OTTSR_STATE_BREAKING) {
        int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
            profile->long_break_minutes : profile->break_minutes;
        display_seconds = (break_minutes * 60) - app->session.elapsed_break_seconds;
        if (break_minutes > 0) {
            progress = (double)app->session.elapsed_break_seconds / (break_minutes * 60);
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->break_progress), 
                                    CLAMP(progress, 0.0, 1.0));
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), 1.0);
    } else {
        display_seconds = profile->study_minutes * 60;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->session_progress), 0.0);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->break_progress), 0.0);
    }
    
    ottsr_format_time(MAX(0, display_seconds), time_str, sizeof(time_str));
    gtk_label_set_text(GTK_LABEL(app->timer_label), time_str);
    
    // Update time spinners only when idle
    if (app->session.state == OTTSR_STATE_IDLE) {
        g_signal_handlers_block_by_func(app->study_time_spin, on_time_changed, app);
        g_signal_handlers_block_by_func(app->break_time_spin, on_time_changed, app);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->study_time_spin), profile->study_minutes);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(app->break_time_spin), profile->break_minutes);
        
        g_signal_handlers_unblock_by_func(app->study_time_spin, on_time_changed, app);
        g_signal_handlers_unblock_by_func(app->break_time_spin, on_time_changed, app);
    }
    
    // Update stats
    char stats_str[256];
    ottsr_format_stats(app, stats_str, sizeof(stats_str));
    gtk_label_set_text(GTK_LABEL(app->stats_label), stats_str);
}

// Timer callback
gboolean ottsr_timer_callback(gpointer user_data) {
    ottsr_app_t *app = (ottsr_app_t *)user_data;
    ottsr_profile_t *profile = &app->config.profiles[app->session.profile_index];
    
    if (app->session.state == OTTSR_STATE_STUDYING) {
        app->session.elapsed_study_seconds++;
        
        if (app->session.elapsed_study_seconds >= profile->study_minutes * 60) {
            // Study session complete, start break
            app->session.current_sessions++;
            app->session.state = OTTSR_STATE_BREAKING;
            app->session.break_start = time(NULL);
            app->session.elapsed_break_seconds = 0;
            
            int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
                profile->long_break_minutes : profile->break_minutes;
                
            gtk_label_set_text(GTK_LABEL(app->status_label), "Break time!");
            
            if (profile->notifications_enabled) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Study session complete! Take a %d minute break.", break_minutes);
                ottsr_show_notification(app, "Session Complete", msg);
            }
            
            if (profile->sound_enabled) {
                ottsr_play_notification_sound(app);
            }
        }
    } else if (app->session.state == OTTSR_STATE_BREAKING) {
        app->session.elapsed_break_seconds++;
        
        int break_minutes = (app->session.current_sessions % profile->sessions_until_long_break == 0) ?
            profile->long_break_minutes : profile->break_minutes;
            
        if (app->session.elapsed_break_seconds >= break_minutes * 60) {
            // Break complete
            if (app->config.autostart_sessions) {
                app->session.state = OTTSR_STATE_STUDYING;
                app->session.session_start = time(NULL);
                app->session.elapsed_study_seconds = 0;
                app->session.break_start = 0;
                gtk_label_set_text(GTK_LABEL(app->status_label), "Next session started automatically");
            } else {
                ottsr_stop_session(app);
                gtk_label_set_text(GTK_LABEL(app->status_label), "Break complete - ready for next session");
                return G_SOURCE_REMOVE;
            }
            
            if (profile->notifications_enabled) {
                ottsr_show_notification(app, "Break Complete", "Ready for the next study session!");
            }
            
            if (profile->sound_enabled) {
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

// Configuration management
gboolean ottsr_load_config(ottsr_app_t *app) {
    char *config_path = ottsr_get_config_path();
    if (!config_path) return FALSE;
    
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    
    if (!json_parser_load_from_file(parser, config_path, &error)) {
        g_warning("Failed to load config: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        g_free(config_path);
        return FALSE;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    JsonObject *obj = json_node_get_object(root);
    
    // Load basic settings
    if (json_object_has_member(obj, "theme"))
        app->config.theme = json_object_get_int_member(obj, "theme");
    if (json_object_has_member(obj, "sound_volume"))
        app->config.sound_volume = json_object_get_int_member(obj, "sound_volume");
    if (json_object_has_member(obj, "minimize_to_tray"))
        app->config.minimize_to_tray = json_object_get_boolean_member(obj, "minimize_to_tray");
    if (json_object_has_member(obj, "autostart_sessions"))
        app->config.autostart_sessions = json_object_get_boolean_member(obj, "autostart_sessions");
    if (json_object_has_member(obj, "active_profile"))
        app->config.active_profile = json_object_get_int_member(obj, "active_profile");
    if (json_object_has_member(obj, "last_subject"))
        strncpy(app->config.last_subject, json_object_get_string_member(obj, "last_subject"), 
                OTTSR_MAX_NAME_LEN - 1);
    
    // Load profiles
    if (json_object_has_member(obj, "profiles")) {
        JsonArray *profiles_array = json_object_get_array_member(obj, "profiles");
        guint len = json_array_get_length(profiles_array);
        
        app->config.profile_count = MIN(len, OTTSR_MAX_PROFILES);
        
        for (guint i = 0; i < app->config.profile_count; i++) {
            JsonObject *profile_obj = json_array_get_object_element(profiles_array, i);
            ottsr_profile_t *profile = &app->config.profiles[i];
            
            if (json_object_has_member(profile_obj, "name"))
                strncpy(profile->name, json_object_get_string_member(profile_obj, "name"), 
                       OTTSR_MAX_NAME_LEN - 1);
            if (json_object_has_member(profile_obj, "study_minutes"))
                profile->study_minutes = json_object_get_int_member(profile_obj, "study_minutes");
            if (json_object_has_member(profile_obj, "break_minutes"))
                profile->break_minutes = json_object_get_int_member(profile_obj, "break_minutes");
            if (json_object_has_member(profile_obj, "long_break_minutes"))
                profile->long_break_minutes = json_object_get_int_member(profile_obj, "long_break_minutes");
            if (json_object_has_member(profile_obj, "sessions_until_long_break"))
                profile->sessions_until_long_break = json_object_get_int_member(profile_obj, "sessions_until_long_break");
            if (json_object_has_member(profile_obj, "sound_enabled"))
                profile->sound_enabled = json_object_get_boolean_member(profile_obj, "sound_enabled");
            if (json_object_has_member(profile_obj, "notifications_enabled"))
                profile->notifications_enabled = json_object_get_boolean_member(profile_obj, "notifications_enabled");
            if (json_object_has_member(profile_obj, "total_study_time"))
                profile->total_study_time = json_object_get_int_member(profile_obj, "total_study_time");
            if (json_object_has_member(profile_obj, "total_sessions"))
                profile->total_sessions = json_object_get_int_member(profile_obj, "total_sessions");
            if (json_object_has_member(profile_obj, "completed_sessions"))
                profile->completed_sessions = json_object_get_int_member(profile_obj, "completed_sessions");
        }
    }
    
    g_object_unref(parser);
    g_free(config_path);
    return TRUE;
}

gboolean ottsr_save_config(ottsr_app_t *app) {
    char *config_path = ottsr_get_config_path();
    if (!config_path) return FALSE;
    
    // Create directory if it doesn't exist
    char *config_dir = g_path_get_dirname(config_path);
    g_mkdir_with_parents(config_dir, 0755);
    g_free(config_dir);
    
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    
    // Save basic settings
    json_builder_set_member_name(builder, "version");
    json_builder_add_string_value(builder, OTTSR_VERSION);
    json_builder_set_member_name(builder, "theme");
    json_builder_add_int_value(builder, app->config.theme);
    json_builder_set_member_name(builder, "sound_volume");
    json_builder_add_int_value(builder, app->config.sound_volume);
    json_builder_set_member_name(builder, "minimize_to_tray");
    json_builder_add_boolean_value(builder, app->config.minimize_to_tray);
    json_builder_set_member_name(builder, "autostart_sessions");
    json_builder_add_boolean_value(builder, app->config.autostart_sessions);
    json_builder_set_member_name(builder, "active_profile");
    json_builder_add_int_value(builder, app->config.active_profile);
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
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);
    json_generator_set_root(generator, root);
    
    GError *error = NULL;
    gboolean success = json_generator_to_file(generator, config_path, &error);
    
    if (!success) {
        g_warning("Failed to save config: %s", error->message);
        g_error_free(error);
    }
    
    g_object_unref(generator);
    g_object_unref(builder);
    g_free(config_path);
    
    return success;
}
