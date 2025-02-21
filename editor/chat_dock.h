#ifndef CHAT_DOCK_H
#define CHAT_DOCK_H

#include "scene/gui/box_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/button.h"
#include "ai_backend.h"
#include "godot_docs_retriever_bind.h"

class ChatDock : public VBoxContainer {
    GDCLASS(ChatDock, VBoxContainer);

private:
    RichTextLabel *chat_display = nullptr;
    LineEdit *input_field = nullptr;
    Button *send_button = nullptr;
    Ref<AIBackend> ai_backend;
    Ref<GodotDocsRetrieverBind> docs_retriever;
    String pending_message;

    void _send_message();
    void _on_input_text_changed(const String &p_text);
    void _on_input_text_submitted(const String &p_text);
    void _on_ai_response(const String &p_response);
    void _on_relevance_response(bool p_is_relevant);
    String _get_docs_context(const String &p_query);
    void _initialize_docs_retriever();

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    ChatDock();
    ~ChatDock();
};

#endif // CHAT_DOCK_H 