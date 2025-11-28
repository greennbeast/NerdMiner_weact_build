#!/bin/bash
cd "$(dirname "$0")"
echo "Starting NerdMiner Dashboard Server..."
echo "Server running at: http://localhost:8000"
echo ""
echo "Opening dashboard in your browser..."
sleep 2
open "http://localhost:8000/nerdminer-dashboard.html"
echo ""
echo "Press Ctrl+C to stop the server"
python3 -m http.server 8000
