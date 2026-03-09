# Virtuelle Umgebung erstellen                                           
python -m venv arcade-led-bridge/.venv
                                                                              
# Aktivieren 
# macOS/Linux:                                                              
source arcade-led-bridge/.venv/bin/activate                                 
  
# Windows CMD:                                                              
arcade-led-bridge\.venv\Scripts\activate.bat

# Windows PowerShell:
arcade-led-bridge\.venv\Scripts\Activate.ps1

# Abhängigkeiten installieren
pip install -r arcade-led-bridge/requirements.txt

**Danach Bridge starten:**

# macOS/Linux
ARCADE_SERIAL_PORT=/dev/cu.SLAB_USBtoUART python arcade-led-bridge/bridge.py

# Windows CMD
set ARCADE_SERIAL_PORT=COM3
python arcade-led-bridge\bridge.py
