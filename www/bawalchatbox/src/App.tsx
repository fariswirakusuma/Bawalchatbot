import { useState, useEffect, useRef } from 'react'

interface Message {
  role: 'user' | 'assistant' | 'system'
  content: string
}

declare global {
  interface Window {
    electronAPI: {
      sendToBackend: (cmd: string) => void
      onBackendOut: (callback: (data: string) => void) => void
      onBackendErr: (callback: (data: string) => void) => void
    }
  }
}

function App(): JSX.Element {
  const [messages, setMessages] = useState<Message[]>([])
  const [input, setInput] = useState('')
  const chatEndRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    window.electronAPI.onBackendOut((data: string) => {
      setMessages((prev) => {
        if (prev.length === 0 || prev[prev.length - 1].role !== 'assistant') {
          return [...prev, { role: 'assistant', content: data }]
        }
        const updated = [...prev]
        updated[updated.length - 1].content += data
        return updated
      })
    })

    window.electronAPI.onBackendErr((data: string) => {
      setMessages((prev) => [...prev, { role: 'system', content: data }])
    })
  }, [])

  useEffect(() => {
    chatEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }, [messages])

  const handleSend = (): void => {
    if (!input.trim()) return

    setMessages((prev) => [...prev, { role: 'user', content: input }])
    window.electronAPI.sendToBackend(input)
    setInput('')
  }

  return (
    <div className="flex flex-col h-screen bg-gray-900 text-gray-100 font-sans">
      <header className="flex items-center justify-between p-4 bg-gray-800 border-b border-gray-700">
        <h1 className="text-xl font-bold text-teal-400 tracking-wide">BawalChatbot</h1>
      </header>

      <main className="flex-1 overflow-y-auto p-4 space-y-4">
        {messages.map((msg, idx) => (
          <div key={idx} className={`flex w-full ${msg.role === 'user' ? 'justify-end' : 'justify-start'}`}>
            <div className={`max-w-2xl p-3 rounded-lg whitespace-pre-wrap shadow-md ${
              msg.role === 'user' ? 'bg-teal-700 text-white' :
              msg.role === 'system' ? 'bg-red-950/80 text-red-200 border border-red-800' : 'bg-gray-800 text-gray-200'
            }`}>
              {msg.content}
            </div>
          </div>
        ))}
        <div ref={chatEndRef} />
      </main>

      <footer className="p-4 bg-gray-800 border-t border-gray-700">
        <div className="flex space-x-2">
          <input
            type="text"
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleSend()}
            placeholder="Ketik perintah..."
            className="flex-1 px-4 py-2 bg-gray-900 border border-gray-600 rounded text-gray-100 focus:outline-none focus:border-teal-500 transition-colors"
          />
          <button
            onClick={handleSend}
            className="px-6 py-2 bg-teal-600 hover:bg-teal-500 text-white font-medium rounded transition-colors shadow-lg"
          >
            Kirim
          </button>
        </div>
      </footer>
    </div>
  )
}

export default App