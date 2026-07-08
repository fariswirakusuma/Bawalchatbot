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
  FLOAT,
  INT,
  BOOL,
  TEXT, 
  END_OF_FILE,
  UNKNOWN,

  PLUS,         // '+'
  MINUS,        // '-'
  STAR,         // '*'
  SLASH,        // '/'
  EQUAL,        // '='
  BANG,         // '!' 

  EQUAL_EQUAL,  // '=='
  BANG_EQUAL,   // '!='
  LESS,         // '<'
  GREATER,      // '>'

  LEFT_PAREN,   // '(' (buat if)
  RIGHT_PAREN,  // ')'
  LEFT_BRACE,   // '{' (awal BlockStatement)
  RIGHT_BRACE,  // '}' (akhir BlockStatement)
  COMMA,        // ','
  
  IF,           // "if"
  ELSE          // "else"                    
}