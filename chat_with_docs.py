from godot_docs_retriever import GodotDocsRetriever

def main():
    # Initialize the retriever
    retriever = GodotDocsRetriever()
    
    print("Godot Documentation Chat Assistant")
    print("Type 'quit' to exit")
    print("-" * 50)
    
    while True:
        # Get user input
        query = input("\nWhat would you like to know about Godot? ").strip()
        
        if query.lower() == 'quit':
            break
        
        if not query:
            continue
        
        try:
            # Search the documentation
            results = retriever.search(query)
            
            # Format and display results
            formatted_results = retriever.format_results(results)
            print("\n" + formatted_results)
            
        except Exception as e:
            print(f"\nError: {str(e)}")
            print("Please try again with a different query.")

if __name__ == "__main__":
    main() 