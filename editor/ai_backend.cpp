#include "ai_backend.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#include "core/string/translation.h"

AIBackend *AIBackend::singleton = nullptr;

void AIBackend::_bind_methods() {
    ClassDB::bind_method(D_METHOD("send_message", "message", "callback"), &AIBackend::send_message);
    ClassDB::bind_method(D_METHOD("clear_history"), &AIBackend::clear_history);
    ClassDB::bind_method(D_METHOD("_http_request_completed"), &AIBackend::_http_request_completed);
    ClassDB::bind_method(D_METHOD("_relevance_request_completed"), &AIBackend::_relevance_request_completed);
    ClassDB::bind_method(D_METHOD("check_godot_relevance", "message", "callback"), &AIBackend::check_godot_relevance);
}

Error AIBackend::initialize() {
    _load_settings();
    
    if (api_key.is_empty()) {
        EditorNode::get_singleton()->show_warning("OpenAI API key not found. Please set it in Editor Settings under Interface > AI.");
        return ERR_UNCONFIGURED;
    }
    
    request = memnew(HTTPRequest);
    request->set_use_threads(true); // Enable threading for better performance
    
    relevance_request = memnew(HTTPRequest);
    relevance_request->set_use_threads(true);
    
    // Use call_deferred to add the child nodes when the tree is ready
    EditorNode::get_singleton()->call_deferred("add_child", request);
    EditorNode::get_singleton()->call_deferred("add_child", relevance_request);
    
    request->connect("request_completed", callable_mp(this, &AIBackend::_http_request_completed));
    relevance_request->connect("request_completed", callable_mp(this, &AIBackend::_relevance_request_completed));
    
    return OK;
}

void AIBackend::_load_settings() {
    api_key = OS::get_singleton()->get_environment("OPENAI_API_KEY");
    print_line(vformat("Trying to load API key from environment: %s", api_key.is_empty() ? "Not found" : "Found"));
    
    if (api_key.is_empty()) {
        Variant api_key_setting = EditorSettings::get_singleton()->get_setting("interface/ai/openai_api_key");
        print_line(vformat("Trying to load API key from editor settings: %s", api_key_setting.get_type() != Variant::NIL ? "Found" : "Not found"));
        if (api_key_setting.get_type() != Variant::NIL) {
            api_key = api_key_setting;
        }
    }
    
    Variant model_setting = EditorSettings::get_singleton()->get_setting("interface/ai/model");
    model = model_setting.get_type() != Variant::NIL ? String(model_setting) : "gpt-3.5-turbo";
    print_line(vformat("Using model: %s", model));
    
    Variant temp_setting = EditorSettings::get_singleton()->get_setting("interface/ai/temperature");
    temperature = temp_setting.get_type() != Variant::NIL ? float(temp_setting) : 0.7f;
    
    Variant tokens_setting = EditorSettings::get_singleton()->get_setting("interface/ai/max_tokens");
    max_tokens = tokens_setting.get_type() != Variant::NIL ? int(tokens_setting) : 1000;
}

void AIBackend::send_message(const String &p_message, const Callable &p_callback) {
    if (!request || !request->is_inside_tree()) {
        ERR_FAIL_MSG("AI Backend not properly initialized or still initializing. Please try again in a moment.");
        return;
    }

    if (api_key.is_empty()) {
        EditorNode::get_singleton()->show_warning("OpenAI API key not found. Please set it in Editor Settings under Interface > AI.");
        return;
    }

    print_line("Sending message to OpenAI API...");
    print_line(vformat("API Key length: %d", api_key.length()));
    print_line(vformat("Message length: %d", p_message.length()));

    message_history.push_back(p_message);
    response_callback = p_callback;
    
    Dictionary messages;
    Array message_array;
    
    // Add system message
    Dictionary system_message;
    system_message["role"] = "system";
    system_message["content"] = "You are a helpful AI assistant integrated into the Godot game engine editor. You help users with game development, coding, and engine-related questions.";
    message_array.push_back(system_message);
    
    // Add message history
    for (const String &msg : message_history) {
        Dictionary user_message;
        user_message["role"] = "user";
        user_message["content"] = msg;
        message_array.push_back(user_message);
    }
    
    Dictionary data;
    data["model"] = model;
    data["messages"] = message_array;
    data["temperature"] = temperature;
    data["max_tokens"] = max_tokens;
    
    String json = JSON::stringify(data);
    print_line(vformat("Request JSON length: %d", json.length()));
    
    Error err = request->request(
        "https://api.openai.com/v1/chat/completions",
        Vector<String>({ 
            "Content-Type: application/json",
            "Authorization: Bearer " + api_key
        }),
        HTTPClient::METHOD_POST,
        json
    );
    
    if (err != OK) {
        String error_msg = vformat("Failed to send request to OpenAI API. Error code: %d", err);
        EditorNode::get_singleton()->show_warning(error_msg);
        return;
    }
    
    print_line("Request sent successfully");
}

void AIBackend::_http_request_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
    if (p_result != HTTPRequest::RESULT_SUCCESS) {
        EditorNode::get_singleton()->show_warning("Failed to connect to OpenAI API.");
        return;
    }
    
    if (p_code != 200) {
        String response_text;
        response_text.parse_utf8((const char *)p_body.ptr(), p_body.size());
        Dictionary response = JSON::parse_string(response_text);
        
        if (response.has("error")) {
            Dictionary error = response["error"];
            EditorNode::get_singleton()->show_warning(vformat("OpenAI API Error: %s", String(error["message"])));
        } else {
            EditorNode::get_singleton()->show_warning(vformat("OpenAI API Error: %d", p_code));
        }
        return;
    }
    
    String response_text;
    response_text.parse_utf8((const char *)p_body.ptr(), p_body.size());
    Dictionary response = JSON::parse_string(response_text);
    
    Array choices = response.get("choices", Array());
    if (choices.size() > 0) {
        Dictionary choice = choices[0];
        Dictionary message = choice.get("message", Dictionary());
        String content = message.get("content", "");
        message_history.push_back(content);
        response_callback.call(content);
    }
}

void AIBackend::clear_history() {
    message_history.clear();
}

void AIBackend::check_godot_relevance(const String &p_message, const Callable &p_callback) {
    if (!relevance_request || !relevance_request->is_inside_tree()) {
        ERR_FAIL_MSG("AI Backend not properly initialized or still initializing.");
        p_callback.call(true); // Default to true if not initialized
        return;
    }

    if (api_key.is_empty()) {
        ERR_FAIL_MSG("OpenAI API key not found.");
        p_callback.call(true); // Default to true if no API key
        return;
    }

    Dictionary data;
    data["model"] = model;
    
    Array message_array;
    
    // Add system message
    Dictionary system_message;
    system_message["role"] = "system";
    system_message["content"] = "You are a classifier that determines if a message is relevant to the Godot game engine and game development. Respond with 'true' for relevant messages and 'false' for irrelevant ones. A message is relevant if it's about:\n1. Godot engine features, APIs, or functionality\n2. Game development concepts or techniques\n3. Game design or implementation using Godot\n4. Technical questions about game development\n5. Game asset creation or management in Godot\n6. Game programming concepts\n7. Game optimization or performance\n8. Game testing and debugging\n9. Game deployment and publishing\n10. Game development workflows\nRespond only with 'true' or 'false'.";
    message_array.push_back(system_message);
    
    // Add user message
    Dictionary user_message;
    user_message["role"] = "user";
    user_message["content"] = p_message;
    message_array.push_back(user_message);
    
    data["messages"] = message_array;
    data["temperature"] = 0.0; // Use 0 temperature for more deterministic responses
    data["max_tokens"] = 10; // We only need a short response
    
    String json = JSON::stringify(data);
    
    relevance_callback = p_callback;
    pending_message = p_message;
    
    Error err = relevance_request->request(
        "https://api.openai.com/v1/chat/completions",
        Vector<String>({ 
            "Content-Type: application/json",
            "Authorization: Bearer " + api_key
        }),
        HTTPClient::METHOD_POST,
        json
    );
    
    if (err != OK) {
        ERR_PRINT("Failed to send relevance check request to OpenAI API.");
        p_callback.call(true); // Default to true on error
    }
}

void AIBackend::_relevance_request_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
    bool is_relevant = true; // Default to true
    
    if (p_result == HTTPRequest::RESULT_SUCCESS && p_code == 200) {
        String response_text;
        response_text.parse_utf8((const char *)p_body.ptr(), p_body.size());
        Dictionary response = JSON::parse_string(response_text);
        
        if (response.has("choices")) {
            Array choices = response["choices"];
            if (choices.size() > 0) {
                Dictionary choice = choices[0];
                Dictionary message = choice["message"];
                String content = String(message["content"]).to_lower();
                is_relevant = content.contains("true");
            }
        }
    }
    
    if (relevance_callback.is_valid()) {
        relevance_callback.call(is_relevant);
    }
}

AIBackend::AIBackend() {
    singleton = this;
}

AIBackend::~AIBackend() {
    if (request) {
        request->queue_free();
    }
    if (relevance_request) {
        relevance_request->queue_free();
    }
    singleton = nullptr;
} 