from langchain_community.vectorstores import Chroma
from langchain_community.embeddings import HuggingFaceEmbeddings
from typing import List, Dict

class GodotDocsRetriever:
    def __init__(self, persist_directory: str = "./chroma_db"):
        self.embeddings = HuggingFaceEmbeddings(model_name="all-MiniLM-L6-v2")
        self.vectorstore = Chroma(
            persist_directory=persist_directory,
            embedding_function=self.embeddings
        )
    
    def search(self, query: str, k: int = 5) -> List[Dict]:
        """
        Search the vectorstore for relevant documentation.
        
        Args:
            query (str): The search query
            k (int): Number of results to return
            
        Returns:
            List[Dict]: List of documents with their content and metadata
        """
        docs = self.vectorstore.similarity_search_with_relevance_scores(query, k=k)
        results = []
        
        for doc, score in docs:
            results.append({
                "content": doc.page_content,
                "metadata": doc.metadata,
                "relevance": score
            })
        
        return results
    
    def format_results(self, results: List[Dict]) -> str:
        """
        Format search results into a readable string.
        
        Args:
            results (List[Dict]): List of search results
            
        Returns:
            str: Formatted string of results
        """
        if not results:
            return "No relevant documentation found."
        
        formatted = "Here are the most relevant sections from the Godot documentation:\n\n"
        
        for i, result in enumerate(results, 1):
            formatted += f"[Result {i}] (Relevance: {result['relevance']:.2f})\n"
            formatted += f"{result['content']}\n\n"
        
        return formatted 