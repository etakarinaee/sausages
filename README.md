
# Install

Debian:
```bash
sudo apt install libfreetype6-dev libglfw3-dev portaudio19-dev luaji
```

Arch:
```bash
sudo pacman -S freetype2 glfw portaudio luajit
```

Fedora:
```bash
sudo dnf install freetype-devel glfw-devel portaudio-devel luajit-devel
```

# Build 
```bash
git clone https://github.com/etakarinaee/sausages.git
cd sausage
mkdir build
cd build
cmake ..
cmake --build .
```

# Usage
```bash
# server 
SAUSAGES_IP= ./server

# client
SAUSAGES_IP= SAUSAGES_PORT= SAUSAGES_NICKNAME= ./client
```

- `SAUSAGES_IP` is IPv4, where the server is hosted and where the client connects, default is `127.0.0.1`
- `SAUSAGES_NICKNAME` is the nickname used in-game, default is `Player`
- `SAUSAGES_PORT` self-explanatory, defaults to 7777

# Gallery

<img width="1110" height="663" alt="image" src="https://github.com/user-attachments/assets/5825d6f9-c405-464b-ba16-fb5b8dff3215" />

