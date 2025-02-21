from langchain_community.embeddings import HuggingFaceEmbeddings
from langchain_community.vectorstores import Chroma
from typing import List, Tuple
import textwrap

def search_docs(query: str, k: int = 5) -> str:
    """
    Search the Godot documentation using semantic search.
    
    Args:
        query: The search query
        k: Number of results to return
        
    Returns:
        A formatted string containing the search results
    """
    # Initialize the same embedding function used to create the vectorstore
    embeddings = HuggingFaceEmbeddings(model_name="all-MiniLM-L6-v2")
    
    # Load the existing vectorstore
    vectorstore = Chroma(
        persist_directory="./chroma_db",
        embedding_function=embeddings
    )
    
    # Perform the search
    results = vectorstore.similarity_search_with_relevance_scores(query, k=k)
    
    # Remove duplicates while preserving order
    seen_content = set()
    unique_results: List[Tuple] = []
    for doc, score in results:
        if doc.page_content not in seen_content:
            seen_content.add(doc.page_content)
            unique_results.append((doc, score))
    
    # Format the results
    formatted_results = []
    for i, (doc, score) in enumerate(unique_results, 1):
        # Wrap text for better readability
        wrapped_content = textwrap.fill(doc.page_content, width=80)
        
        formatted_results.append(
            f"\nResult {i} (Relevance Score: {score:.4f})"
            f"\n{'-' * 80}\n"
            f"Content:\n{wrapped_content}\n"
            f"\nSource: {doc.metadata.get('source', 'Unknown')}\n"
            f"{'=' * 80}"
        )
    
    if not formatted_results:
        return "No results found."
    
    return "\n".join(formatted_results)

def main():
    print("=" * 80)
    print("Godot Documentation Search".center(80))
    print("=" * 80)
    print("\nEnter your search queries below. Type 'quit' to exit.")
    print("Tip: Be specific in your queries for better results!")
    
    while True:
        try:
            query = input("\nQuery: ").strip()
            if query.lower() in ('quit', 'exit', 'q'):
                print("\nThank you for using the Godot Documentation Search!")
                break
                
            if not query:
                print("Please enter a valid query.")
                continue
                
            print("\nSearching...")
            results = search_docs(query)
            print("\nSearch Results:")
            print(results)
            
        except KeyboardInterrupt:
            print("\n\nSearch interrupted. Goodbye!")
            break
        except Exception as e:
            print(f"An error occurred: {e}")

if __name__ == "__main__":
    main() 