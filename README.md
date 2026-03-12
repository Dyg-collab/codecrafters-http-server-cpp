[![progress-banner](https://backend.codecrafters.io/progress/http-server/b16220d0-d32e-4c39-a0d9-534b8e7eebdd)](https://app.codecrafters.io/users/Dyg-collab?r=2qF)

# HTTP Server (C++)

[![build passing](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/)
[![language C++](https://img.shields.io/badge/language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![license MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A concise, from-scratch HTTP/1.1 server implemented in modern C++ showcasing low-level socket programming, request parsing, file serving, and simple concurrency.

Built as part of the **Build Your Own HTTP Server** challenge on CodeCrafters.

---

## Why this project

This repo is a compact, focused demonstration of systems-level skills valuable for internships and junior systems roles:

- networking fundamentals (sockets, TCP)  
- protocol parsing (HTTP request line, headers, body)  
- filesystem interaction (serve and create files)  
- concurrency basics (per-connection threads)  
- clear commit-driven progression (stage-based development)

---

## Highlights

- Clean, single-file entry point: `src/main.cpp` (easy to read / run)
- Implements core HTTP features and routing:
  - `/` root
  - `/echo/{text}`
  - `/files/{filename}` (GET → serve file)
- Concurrent connections via `std::thread`
- Stage-based workflow (follows CodeCrafters stages for rapid iteration)
- Minimal external dependencies — pure STL + POSIX sockets

---

## Quickstart

```bash
# clone
git clone https://github.com/Dyg-collab/codecrafters-http-server-cpp.git
cd codecrafters-http-server-cpp

# build (single-file)
g++ -std=c++17 src/main.cpp -O2 -o http-server

# run (serve files from /tmp/http-files)
mkdir -p /tmp/http-files
./http-server --directory /tmp/http-files --port 4221
