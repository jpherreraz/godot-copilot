#ifndef AI_BACKEND_H
#define AI_BACKEND_H

#include "core/object/ref_counted.h"
#include "core/templates/list.h"
#include "core/variant/variant.h"
#include "scene/main/http_request.h"

class AIBackend : public RefCounted {
    GDCLASS(AIBackend, RefCounted);

private:
    static AIBackend *singleton;
    
    String api_key;
    String model;
    float temperature;
    int max_tokens;
    
    HTTPRequest *request = nullptr;
    HTTPRequest *relevance_request = nullptr;
    List<String> message_history;
    Callable response_callback;
    Callable relevance_callback;
    String pending_message;
    
    void _send_request(const String &p_message);
    void _http_request_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body);
    void _relevance_request_completed(int p_result, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_body);
    void _load_settings();
    
protected:
    static void _bind_methods();
    
public:
    static AIBackend *get_singleton() { return singleton; }
    
    Error initialize();
    void send_message(const String &p_message, const Callable &p_callback);
    void check_godot_relevance(const String &p_message, const Callable &p_callback);
    void clear_history();
    
    AIBackend();
    ~AIBackend();
};

#endif // AI_BACKEND_H 