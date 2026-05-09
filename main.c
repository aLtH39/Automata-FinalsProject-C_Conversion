#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dfa.h"
#include "tokenizer.h"

static GtkWidget *input_text;
static GtkWidget *output_text;
static gboolean   placeholder_active = TRUE;

static void set_output(const char *text) {
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_text));
    gtk_text_buffer_set_text(buffer, text, -1);
}

static void validate_text(const char *raw) {
    if (raw == NULL || strlen(raw) == 0) {
        set_output("Please enter an instruction.");
        return;
    }
    if (strcmp(raw, "Type here...") == 0) {
        set_output("Please enter an instruction.");
        return;
    }

    char  result[16384] = "";
    const char *p = raw;

    while (*p != '\0') {
        char line[512] = "";
        int  i = 0;
        while (*p != '\0' && *p != '\n' && *p != '\r') {
            if (i < (int)sizeof(line) - 1) line[i++] = *p;
            p++;
        }
        line[i] = '\0';
        if (*p == '\r') p++;
        if (*p == '\n') p++;

        int len = (int)strlen(line);
        while (len > 0 && line[len-1] == ' ') line[--len] = '\0';
        if (len == 0) continue;

        int   valid     = semantic_validate(line);
        Token tokens[MAX_TOKENS];
        int   tok_count = tokenize(line, tokens);

        char tok_str[2048] = "";
        for (int j = 0; j < tok_count; j++) {
            strncat(tok_str, token_to_string(tokens[j]), sizeof(tok_str) - strlen(tok_str) - 1);
            strncat(tok_str, "  ", sizeof(tok_str) - strlen(tok_str) - 1);
        }

        strncat(result, "Input   : ", sizeof(result) - strlen(result) - 1);
        strncat(result, line,         sizeof(result) - strlen(result) - 1);
        strncat(result, "\nTokens  : ", sizeof(result) - strlen(result) - 1);
        strncat(result, tok_str,      sizeof(result) - strlen(result) - 1);
        strncat(result, "\nResult  : ", sizeof(result) - strlen(result) - 1);
        strncat(result, valid ? "VALID" : "INVALID", sizeof(result) - strlen(result) - 1);
        strncat(result, "\n", sizeof(result) - strlen(result) - 1);

        if (!valid) {
            strncat(result, "Hint    : ", sizeof(result) - strlen(result) - 1);
            strncat(result, hint(line, tokens, tok_count), sizeof(result) - strlen(result) - 1);
            strncat(result, "\n", sizeof(result) - strlen(result) - 1);
        }
        strncat(result, "\n", sizeof(result) - strlen(result) - 1);
    }

    set_output(result);
}

static void set_placeholder(void) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text));
    gtk_text_buffer_set_text(buf, "Type here...", -1);
    GdkRGBA grey = {0.5, 0.5, 0.5, 1.0};
    gtk_widget_override_color(input_text, GTK_STATE_FLAG_NORMAL, &grey);
    placeholder_active = TRUE;
}

static void clear_placeholder(void) {
    if (placeholder_active) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text));
        gtk_text_buffer_set_text(buf, "", -1);
        GdkRGBA black = {0.0, 0.0, 0.0, 1.0};
        gtk_widget_override_color(input_text, GTK_STATE_FLAG_NORMAL, &black);
        placeholder_active = FALSE;
    }
}

static gboolean on_input_focus_in(GtkWidget *widget, GdkEvent *event, gpointer data) {
    (void)widget; (void)event; (void)data;
    clear_placeholder();
    return FALSE;
}

static gboolean on_input_focus_out(GtkWidget *widget, GdkEvent *event, gpointer data) {
    (void)widget; (void)event; (void)data;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
    if (strlen(text) == 0) set_placeholder();
    g_free(text);
    return FALSE;
}

static void on_input_changed(GtkTextBuffer *buffer, gpointer data) {
    (void)buffer; (void)data;
    if (!placeholder_active) {
        set_output("");
    }
}

static void on_validate_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (placeholder_active) {
        set_output("Please enter an instruction.");
        return;
    }
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    validate_text(text);
    g_free(text);
}

static void on_open_clicked(GtkWidget *widget, gpointer window) {
    (void)widget;
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open",   GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        FILE *file = fopen(filename, "r");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            long fsize = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *content = (char *)malloc(fsize + 1);
            if (content != NULL) {
                size_t read = fread(content, 1, fsize, file);
                content[read] = '\0';

                GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text));
                gtk_text_buffer_set_text(buf, content, -1);

                GdkRGBA black = {0.0, 0.0, 0.0, 1.0};
                gtk_widget_override_color(input_text, GTK_STATE_FLAG_NORMAL, &black);
                placeholder_active = FALSE;

                validate_text(content);
                free(content);
            }
            fclose(file);
        } else {
            GtkWidget *err = gtk_message_dialog_new(
                GTK_WINDOW(window), GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Failed to read file."
            );
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Opcode Validator");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
    gtk_container_add(GTK_CONTAINER(window), grid);

    input_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(input_text), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_tooltip_text(input_text, "Enter an instruction, e.g. MOV R1, 100");

    GtkWidget *scroll1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll1),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll1, -1, 200);
    gtk_container_add(GTK_CONTAINER(scroll1), input_text);

    set_placeholder();
    g_signal_connect(input_text, "focus-in-event",  G_CALLBACK(on_input_focus_in),  NULL);
    g_signal_connect(input_text, "focus-out-event", G_CALLBACK(on_input_focus_out), NULL);
    g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_text)),
                 "changed", G_CALLBACK(on_input_changed), NULL);

    output_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_text), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_text), GTK_WRAP_WORD_CHAR);

    GtkWidget *scroll2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll2),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll2, -1, 300);
    gtk_container_add(GTK_CONTAINER(scroll2), output_text);

    GtkWidget *validate_button = gtk_button_new_with_label("Validate");
    GtkWidget *open_button     = gtk_button_new_with_label("Open File");

    g_signal_connect(validate_button, "clicked", G_CALLBACK(on_validate_clicked), NULL);
    g_signal_connect(open_button,     "clicked", G_CALLBACK(on_open_clicked),     window);

    gtk_grid_attach(GTK_GRID(grid), scroll1,         0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), validate_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), open_button,     1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scroll2,         0, 2, 2, 1);

    gtk_widget_set_hexpand(scroll1, TRUE);
    gtk_widget_set_vexpand(scroll1, TRUE);
    gtk_widget_set_hexpand(scroll2, TRUE);
    gtk_widget_set_vexpand(scroll2, TRUE);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}