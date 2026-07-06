export interface ContextSession {
  systemPrompt: string;           
  currentTemperature: number;     
  maxTokens: number;              
  topK: number;                   
  topP: number;                   
  repeatPenalty: number;          
  shouldExit: boolean;            
  chatHistory: Message[];         
  modelName: string;              
  historyFileName: string;        
}

export interface Message {
  role: string;                   
  content: string;                
}

export interface StructuredOutput {
  rawText: string;                
  tokensGenerated: number;        
  inferenceTimeMs: number;        
  executionStatus: string;        
}

export enum TokenType {
  COMMAND,                        
  PARAMETER,                      
  TEXT,                           
  END_OF_FILE,                    
  UNKNOWN                         
}