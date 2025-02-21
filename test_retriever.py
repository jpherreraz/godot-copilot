from godot_docs_retriever import GodotDocsRetriever
import sys

print("Python version:", sys.version)
print("Python executable:", sys.executable)
print("Python path:", sys.path)

try:
    retriever = GodotDocsRetriever()
    results = retriever.search("How do I create a new node in Godot?", k=3)
    print("\nSearch Results:")
    print(retriever.format_results(results))
except Exception as e:
    print("Error:", str(e))
    import traceback
    traceback.print_exc() 