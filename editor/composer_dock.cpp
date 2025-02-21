#include "composer_dock.h"

#include "core/config/project_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/editor_string_names.h"
#include "editor/editor_node.h"

void ComposerDock::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            if (composer_display) {
                composer_display->add_theme_font_override("normal_font", get_theme_font(SNAME("output_source"), EditorStringName(EditorFonts)));
                composer_display->add_theme_font_size_override("normal_font_size", get_theme_font_size(SNAME("output_source_size"), EditorStringName(EditorFonts)));
                composer_display->add_theme_constant_override("line_separation", 4 * EDSCALE);
            }
            if (send_button) {
                send_button->set_button_icon(get_theme_icon(SNAME("Forward"), EditorStringName(EditorIcons)));
            }
        } break;

        case NOTIFICATION_ENTER_TREE: {
            if (input_field) {
                input_field->grab_focus();
            }
            
            // Initialize AI backend
            ai_backend = Ref<AIBackend>(memnew(AIBackend));
            Error err = ai_backend->initialize();
            if (err != OK) {
                composer_display->add_text("Error: Failed to initialize AI backend. Please check your settings.\n");
            }

            // Initialize docs retriever
            _initialize_docs_retriever();
        } break;
    }
}

void ComposerDock::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_send_message"), &ComposerDock::_send_message);
    ClassDB::bind_method(D_METHOD("_on_input_text_changed", "text"), &ComposerDock::_on_input_text_changed);
    ClassDB::bind_method(D_METHOD("_on_input_text_submitted", "text"), &ComposerDock::_on_input_text_submitted);
    ClassDB::bind_method(D_METHOD("_on_ai_response", "response"), &ComposerDock::_on_ai_response);
    ClassDB::bind_method(D_METHOD("_on_relevance_response", "is_relevant"), &ComposerDock::_on_relevance_response);
}

void ComposerDock::_initialize_docs_retriever() {
    docs_retriever = Ref<GodotDocsRetrieverBind>(memnew(GodotDocsRetrieverBind));
    Error err = docs_retriever->initialize();
    if (err != OK) {
        composer_display->add_text("Warning: Failed to initialize documentation retriever. Documentation context will not be available.\n");
    }
}

String ComposerDock::_get_docs_context(const String &p_query) {
    if (docs_retriever.is_valid()) {
        Array results = docs_retriever->search(p_query);
        if (results.size() > 0) {
            String formatted_results = docs_retriever->format_results(results);
            if (!formatted_results.is_empty()) {
                ERR_PRINT("Formatted documentation results:\n" + formatted_results);
                return formatted_results;
            }
        }
    }
    return String();
}

void ComposerDock::_send_message() {
    String message = input_field->get_text().strip_edges();
    if (!message.is_empty()) {
        composer_display->add_text("You: " + message + "\n");
        input_field->clear();
        
        if (ai_backend.is_valid()) {
            composer_display->add_text("AI: Thinking...\n");
            
            // Store message for use in callback
            pending_message = message;
            
            // Check if the message is relevant to Godot
            ai_backend->check_godot_relevance(message, callable_mp(this, &ComposerDock::_on_relevance_response));
        } else {
            composer_display->add_text("AI: Error - AI backend not initialized.\n");
        }
    }
}

void ComposerDock::_on_relevance_response(bool p_is_relevant) {
    // Get documentation context only if the message is relevant
    String docs_context;
    if (p_is_relevant) {
        docs_context = _get_docs_context(pending_message);
    }
    
    // Combine user message with documentation context if relevant
    String enhanced_message;
    if (p_is_relevant && !docs_context.is_empty()) {
        enhanced_message = "User Query: " + pending_message + "\n\n";
        enhanced_message += "Relevant Godot Documentation:\n" + docs_context + "\n\n";
        enhanced_message += "Please use the above documentation context to help answer this question: " + pending_message;
    } else {
        enhanced_message = pending_message;
    }
    
    // Log the enhanced message being sent to the LLM
    ERR_PRINT("Sending to LLM (ComposerDock):\n" + enhanced_message);
    
    ai_backend->send_message(enhanced_message, callable_mp(this, &ComposerDock::_on_ai_response));
}

void ComposerDock::_on_ai_response(const String &p_response) {
    // Remove only the last line containing "Thinking..."
    String current_text = composer_display->get_text();
    int thinking_pos = current_text.rfind("AI: Thinking...\n");
    if (thinking_pos != -1) {
        composer_display->remove_paragraph(composer_display->get_paragraph_count() - 2);
    }
    
    // Add the actual response
    composer_display->add_text("AI: " + p_response + "\n");
}

void ComposerDock::_on_input_text_changed(const String &p_text) {
    send_button->set_disabled(p_text.strip_edges().is_empty());
}

void ComposerDock::_on_input_text_submitted(const String &p_text) {
    _send_message();
}

ComposerDock::ComposerDock() {
    set_name("Composer");
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 4 * EDSCALE);

    // Composer display area
    composer_display = memnew(RichTextLabel);
    composer_display->set_v_size_flags(SIZE_EXPAND_FILL);
    composer_display->set_focus_mode(Control::FOCUS_NONE);
    composer_display->set_selection_enabled(true);
    composer_display->set_context_menu_enabled(true);
    composer_display->set_scroll_follow(true);
    composer_display->set_custom_minimum_size(Size2(0, 100) * EDSCALE);
    composer_display->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(composer_display);

    // Input area container
    HBoxContainer *input_hbox = memnew(HBoxContainer);
    input_hbox->add_theme_constant_override("separation", 4 * EDSCALE);
    add_child(input_hbox);

    // Text input field
    input_field = memnew(LineEdit);
    input_field->set_h_size_flags(SIZE_EXPAND_FILL);
    input_field->set_placeholder("Type your message here...");
    input_field->connect("text_changed", callable_mp(this, &ComposerDock::_on_input_text_changed));
    input_field->connect("text_submitted", callable_mp(this, &ComposerDock::_on_input_text_submitted));
    input_hbox->add_child(input_field);

    // Send button
    send_button = memnew(Button);
    send_button->set_disabled(true);
    send_button->set_flat(true);
    send_button->connect("pressed", callable_mp(this, &ComposerDock::_send_message));
    input_hbox->add_child(send_button);

    // Initial welcome message
    composer_display->add_text("Welcome to the Godot AI Composer! How can I help you today?\n");
}

ComposerDock::~ComposerDock() {
    // No manual cleanup needed since nodes are freed automatically
} 