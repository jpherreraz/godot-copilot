#ifndef GODOT_DOCS_RETRIEVER_BIND_H
#define GODOT_DOCS_RETRIEVER_BIND_H

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/array.h"

class GodotDocsRetrieverBind : public RefCounted {
    GDCLASS(GodotDocsRetrieverBind, RefCounted);

protected:
    static void _bind_methods();

public:
    GodotDocsRetrieverBind();
    ~GodotDocsRetrieverBind();

    Array search(const String &query, int k = 5);
    String format_results(const Array &results);
    Error initialize();

private:
    String _run_python_script(const String &script, const Array &args);
};

#endif // GODOT_DOCS_RETRIEVER_BIND_H 