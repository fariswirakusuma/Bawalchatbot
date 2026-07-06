import React, { useState, useEffect, useRef } from 'react';
import './style/App.css'; 

declare global {
  interface Window {
    electronAPI: {
      sendToBackend: (cmd: string) => void;
      onBackendOut: (callback: (data: string) => void) => () => void;
      onBackendErr: (callback: (data: string) => void) => () => void;
    };
  }
}

const VALID_COMMANDS = [
  '/help', '/generate', '/set_model', '/set_prompt', 
  '/load_history', '/save_history', '/set_param', '/exit'
];

const formatMessageWithRegex = (text: string) => {
  if (!text) return null;
  const commandRegex = /(\/[a-zA-Z0-9_]+)/g;
  const parts = text.split(commandRegex);

  return parts.map((part, index) => {
    if (part.match(commandRegex)) {
      return (
        <span key={index} className="cmd-highlight">
          {part}
        </span>
      );
    }
    return <span key={index}>{part}</span>;
  });
};

export default function App() {
  const [inputText, setInputText] = useState('');
  const [suggestions, setSuggestions] = useState<string[]>([]);
  const [messages, setMessages] = useState<{role: string, content: string}[]>([]);
  const endOfMessagesRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    endOfMessagesRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  useEffect(() => {
    if (window.electronAPI) {
      const removeOutListener = window.electronAPI.onBackendOut((data: string) => {
        const lines = data.split('\n').filter(line => line.trim() !== '');
        
        for (const line of lines) {
          try {
            const parsed = JSON.parse(line);
            
            if (parsed.status === 'info' || parsed.status === 'error') {
              setMessages(prev => [...prev, { 
                role: parsed.status === 'error' ? 'error' : 'system', 
                content: parsed.message.replace(/\\n/g, '\n')
              }]);
            } 
            else if (parsed.status === 'start') {
              setMessages(prev => [...prev, { role: 'assistant', content: '' }]);
            } 
            else if (parsed.status === 'token') {
              setMessages(prev => {
                const newMsgs = [...prev];
                const lastIdx = newMsgs.length - 1;
                if (lastIdx >= 0 && newMsgs[lastIdx].role === 'assistant') {
                  newMsgs[lastIdx].content += parsed.content.replace(/\\n/g, '\n');
                }
                return newMsgs;
              });
            }
          } catch (e) {
            console.log("Log Non-JSON C++:", line);
          }
        }
      });

      const removeErrListener = window.electronAPI.onBackendErr((data: string) => {
        console.error("Backend Error:", data);
      });

      // Cleanup listener saat komponen unmount
      return () => {
        removeOutListener();
        removeErrListener();
      };
    }
  }, []);

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const value = e.target.value;
    setInputText(value);

    if (value.startsWith('/') && !value.includes(' ')) {
      const matches = VALID_COMMANDS.filter(cmd => cmd.startsWith(value));
      setSuggestions(matches);
    } else {
      setSuggestions([]);
    }
  };

  const applySuggestion = (cmd: string) => {
    setInputText(cmd + ' ');
    setSuggestions([]);
    document.getElementById('chat-input')?.focus();
  };

  const handleSend = () => {
    if (!inputText.trim()) return;
    
    setMessages(prev => [...prev, { role: 'user', content: inputText }]);
    
    if (window.electronAPI) {
      window.electronAPI.sendToBackend(inputText);
    }
    
    setInputText('');
    setSuggestions([]);
  };

  return (
    <div className="app-container">
      {/* Header Baru dengan Judul, Versi, dan Animasi */}
      <header className="app-header">
        <div className="logo-container">
          <h1 className="app-title">
            Bawal<span className="title-highlight">chatbox</span>
          </h1>
          <span className="app-version">v0.2.0</span>
        </div>
        <div className="header-status">
          <span className="status-dot"></span>
          <span className="status-text">Engine Ready</span>
        </div>
      </header>

      <div className="chat-area">
        {messages.length === 0 && (
          <div className="welcome-container">
            <div className="welcome-icon">🐟</div>
            <h2>Selamat Datang di Bawalchatbox</h2>
            <p>Ketik perintah seperti <span className="cmd-highlight">/help</span> atau langsung kirim pesan untuk memulai interaksi dengan model lokal.</p>
          </div>
        )}
        {messages.map((msg, idx) => (
          <div key={idx} className={`message-row ${msg.role === 'user' ? 'message-user' : 'message-bot'}`}>
            <div className={`message-bubble ${
              msg.role === 'user' ? 'bubble-user' : 
              msg.role === 'error' ? 'bubble-error' : 'bubble-system'
            }`}>
              {formatMessageWithRegex(msg.content)}
            </div>
          </div>
        ))}
        <div ref={endOfMessagesRef} />
      </div>

      <div className="input-area">
        {suggestions.length > 0 && (
          <div className="suggestion-box">
            {suggestions.map((cmd, idx) => (
              <button
                key={idx}
                onClick={() => applySuggestion(cmd)}
                className="suggestion-btn"
              >
                {cmd}
              </button>
            ))}
          </div>
        )}

        <div className="input-wrapper">
          <input
            id="chat-input"
            type="text"
            value={inputText}
            onChange={handleInputChange}
            onKeyDown={(e) => {
              if (e.key === 'Enter') handleSend();
              if (e.key === 'Tab' && suggestions.length > 0) {
                e.preventDefault();
                applySuggestion(suggestions[0]);
              }
            }}
            placeholder="Ketik perintah (contoh: /generate) atau pesan..."
            className="input-field"
          />
          <button onClick={handleSend} className="btn-send">
            Kirim
          </button>
        </div>
      </div>
    </div>
  );
}
