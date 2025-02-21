#include "godot_docs_retriever_bind.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/error/error_macros.h"
#include "core/os/os.h"

void GodotDocsRetrieverBind::_bind_methods() {
    ClassDB::bind_method(D_METHOD("search", "query", "k"), &GodotDocsRetrieverBind::search, DEFVAL(5));
    ClassDB::bind_method(D_METHOD("format_results", "results"), &GodotDocsRetrieverBind::format_results);
    ClassDB::bind_method(D_METHOD("initialize"), &GodotDocsRetrieverBind::initialize);
}

GodotDocsRetrieverBind::GodotDocsRetrieverBind() {
}

GodotDocsRetrieverBind::~GodotDocsRetrieverBind() {
}

String GodotDocsRetrieverBind::_run_python_script(const String &script, const Array &args) {
    ERR_PRINT("Starting Python script execution...");
    
    String python_path = OS::get_singleton()->get_environment("VIRTUAL_ENV");
    ERR_PRINT("VIRTUAL_ENV: " + python_path);
    
    if (python_path.is_empty()) {
        ERR_PRINT("VIRTUAL_ENV not set, using default python3");
        python_path = "python3";
    } else {
        python_path = python_path + "/bin/python3";
        ERR_PRINT("Using Python from virtualenv: " + python_path);
    }

    // Get the executable path and use its directory
    String exec_path = OS::get_singleton()->get_executable_path();
    String base_dir = exec_path.get_base_dir();
    String project_root = base_dir.get_base_dir();
    ERR_PRINT("Executable directory: " + base_dir);
    ERR_PRINT("Project root: " + project_root);

    // Set up environment variables
    OS::get_singleton()->set_environment("PYTHONPATH", project_root);
    OS::get_singleton()->set_environment("PYTHONUNBUFFERED", "1");
    ERR_PRINT("Setting PYTHONPATH: " + project_root);

    List<String> args_list;
    args_list.push_back("-c");
    
    // Add more debug output to the Python script
    String modified_script = R"(
import os
import sys
import traceback
import json

def log(message, is_error=False):
    try:
        output = {
            'type': 'error' if is_error else 'debug',
            'message': str(message)
        }
        print(json.dumps(output), flush=True)
    except Exception as e:
        print('Error in log function: ' + str(e), file=sys.stderr, flush=True)

def main():
    try:
        # Configure Python environment
        os.environ['PYTHONUNBUFFERED'] = '1'
        if os.getcwd() not in sys.path:
            sys.path.insert(0, os.getcwd())

        # Log environment information
        log('Starting Python script')
        log('Current working directory: {}'.format(os.getcwd()))
        log('PYTHONPATH: {}'.format(os.environ.get('PYTHONPATH', 'Not set')))
        log('Python executable: {}'.format(sys.executable))
        log('Python version: {}'.format(sys.version))
        log('Python path: {}'.format(sys.path))

        # User script starts here
)";
    // Add proper indentation to the user's script
    Vector<String> script_lines = script.split("\n");
    for (int i = 0; i < script_lines.size(); i++) {
        String line = script_lines[i];
        if (!line.strip_edges().is_empty()) {
            modified_script += "        " + line + "\n";
        } else {
            modified_script += "\n";
        }
    }
    modified_script += R"(
    except Exception as e:
        log(str(e), True)
        log(traceback.format_exc(), True)
        sys.exit(1)

if __name__ == '__main__':
    main()
)";
    
    args_list.push_back(modified_script);
    for (int i = 0; i < args.size(); i++) {
        args_list.push_back(args[i]);
    }

    String output;
    int exit_code = 0;
    ERR_PRINT("Executing Python command: " + python_path);
    Error err = OS::get_singleton()->execute(python_path, args_list, &output, &exit_code, true);
    
    if (err != OK) {
        ERR_PRINT("Failed to execute Python script: " + itos(err));
        return String();
    }
    
    ERR_PRINT("Python script output: " + output);
    ERR_PRINT("Python script exit code: " + itos(exit_code));
    
    if (exit_code != 0) {
        ERR_PRINT("Python script execution failed with code " + itos(exit_code) + ": " + output);
        return String();
    }

    // Find the last valid JSON object in the output
    Vector<String> lines = output.split("\n");
    String last_json;
    for (int i = lines.size() - 1; i >= 0; i--) {
        String line = lines[i].strip_edges();
        if (line.is_empty()) continue;
        
        Variant json_result = JSON::parse_string(line);
        if (json_result.get_type() == Variant::DICTIONARY) {
            Dictionary dict = json_result;
            if (dict.has("type")) {
                if (String(dict["type"]) == "success") {
                    return line;
                } else if (String(dict["type"]) == "result" && dict.has("message")) {
                    return line;
                }
            }
        }
    }
    
    return String();
}

Error GodotDocsRetrieverBind::initialize() {
    ERR_PRINT("Initializing GodotDocsRetrieverBind...");
    
    String script = R"(
# Initialize the retriever
log('Starting initialization')
log('Current working directory: {}'.format(os.getcwd()))
log('PYTHONPATH: {}'.format(os.environ.get('PYTHONPATH', 'Not set')))
log('Sys path: {}'.format(sys.path))

log('Attempting to import GodotDocsRetriever')
from godot_docs_retriever import GodotDocsRetriever
log('Successfully imported GodotDocsRetriever')

log('Attempting to initialize GodotDocsRetriever')
retriever = GodotDocsRetriever()
log('Successfully initialized GodotDocsRetriever')

print(json.dumps({'type': 'success', 'message': 'OK'}), flush=True)
)";

    String result = _run_python_script(script, Array());
    ERR_PRINT("Initialization result: " + result);
    
    // Parse the JSON output to check for success
    Variant json_result = JSON::parse_string(result);
    if (json_result.get_type() == Variant::DICTIONARY) {
        Dictionary dict = json_result;
        if (dict.has("type") && String(dict["type"]) == "success") {
            return OK;
        }
    }
    return ERR_CANT_CREATE;
}

Array GodotDocsRetrieverBind::search(const String &query, int k) {
    String script = R"(
# Search the documentation
from godot_docs_retriever import GodotDocsRetriever

retriever = GodotDocsRetriever()
results = retriever.search(sys.argv[1], int(sys.argv[2]))
print(json.dumps({
    'type': 'result',
    'message': [{
        'content': r['content'],
        'metadata': r['metadata'],
        'relevance': r['relevance']
    } for r in results]
}))
)";

    Array args;
    args.push_back(query);
    args.push_back(String::num_int64(k));

    String output = _run_python_script(script, args);
    if (output.is_empty()) {
        return Array();
    }

    // Parse the JSON output to get the results
    Variant json_result = JSON::parse_string(output);
    if (json_result.get_type() == Variant::DICTIONARY) {
        Dictionary dict = json_result;
        if (dict.has("type") && String(dict["type"]) == "result") {
            return dict["message"];
        }
    }

    return Array();
}

String GodotDocsRetrieverBind::format_results(const Array &results) {
    if (results.is_empty()) {
        return String();
    }

    String formatted = "Here are the most relevant sections from the Godot documentation:\n\n";
    HashSet<String> seen_content;
    
    for (int i = 0; i < results.size(); i++) {
        Dictionary result = results[i];
        if (result.has("content") && result.has("relevance")) {
            String content = String(result["content"]);
            if (!seen_content.has(content)) {
                seen_content.insert(content);
                formatted += vformat("[Result %d] (Relevance: %.2f)\n", seen_content.size(), (double)result["relevance"]);
                formatted += content + "\n\n";
            }
        }
    }
    
    return formatted;
} 