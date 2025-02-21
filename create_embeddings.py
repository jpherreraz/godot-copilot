from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain_community.document_loaders import TextLoader
from langchain_community.vectorstores import Chroma
from langchain_community.embeddings import HuggingFaceEmbeddings

def create_vectorstore():
    # Initialize embeddings
    embeddings = HuggingFaceEmbeddings(model_name="all-MiniLM-L6-v2")
    
    # Load the document
    loader = TextLoader("godot_docs.txt")
    documents = loader.load()
    
    # Create text splitter
    text_splitter = RecursiveCharacterTextSplitter(
        chunk_size=1000,
        chunk_overlap=200,
        length_function=len,
        is_separator_regex=False,
    )
    
    # Split the documents
    splits = text_splitter.split_documents(documents)
    
    # Create and persist the vectorstore
    vectorstore = Chroma.from_documents(
        documents=splits,
        embedding=embeddings,
        persist_directory="./chroma_db"
    )
    
    # Persist the vectorstore
    vectorstore.persist()
    print(f"Created vectorstore with {len(splits)} chunks")
    return vectorstore

if __name__ == "__main__":
    create_vectorstore()