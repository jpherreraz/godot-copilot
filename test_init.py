import os
import sys
import json
import traceback

def log(message, is_error=False):
    output = {
        'type': 'error' if is_error else 'debug',
        'message': str(message)
    }
    print(json.dumps(output), flush=True)

try:
    log('Starting Python script')
    log('Current working directory: {}'.format(os.getcwd()))
    log('PYTHONPATH: {}'.format(os.environ.get('PYTHONPATH', 'Not set')))
    log('Python executable: {}'.format(sys.executable))
    log('Python version: {}'.format(sys.version))
    log('Python path: {}'.format(sys.path))

    from godot_docs_retriever import GodotDocsRetriever
    log('Successfully imported GodotDocsRetriever')
    
    retriever = GodotDocsRetriever()
    log('Successfully initialized GodotDocsRetriever')
    
    print(json.dumps({'type': 'success', 'message': 'OK'}), flush=True)
except Exception as e:
    log(str(e), True)
    log(traceback.format_exc(), True)
    sys.exit(1) 