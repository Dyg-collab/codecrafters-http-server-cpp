# HTTP Server (C++)

![build passing](https://img.shields.io/badge/build-passing-brightgreen.svg)
![language C++](https://img.shields.io/badge/language-C%2B%2B-blue.svg)
![license MIT](https://img.shields.io/badge/license-MIT-blue.svg)

A concise, from-scratch HTTP/1.1 server implemented in modern C++ showcasing low-level socket programming, request parsing, persistent connections, gzip compression, file serving, and simple concurrency.

Built as part of the **Build Your Own HTTP Server** challenge on CodeCrafters.

---

## Why this project

This repo is a compact, focused demonstration of systems-level skills valuable for internships and junior systems roles:

- networking fundamentals (sockets, TCP)
- protocol parsing (HTTP request line, headers, body, `Content-Length`-driven reads)
- persistent connections (HTTP Keep-Alive)
- filesystem interaction (serve and create files, GET/POST)
- concurrency basics (per-connection threads)
- clear commit-driven progression (stage-based development)

---

## Highlights

- Clean, single-file entry point: `src/main.cpp` (easy to read / run)
- Implements core HTTP features and routing:
  * `/` root
  * `/echo/{text}` — with on-demand gzip compression via zlib when the client sends `Accept-Encoding: gzip`
  * `/user-agent` — echoes the client's `User-Agent` header
  * `/files/{filename}` — `GET` to serve a file, `POST` to upload/write one
- Persistent connections (HTTP/1.1 Keep-Alive): multiple requests are served over a single TCP connection unless the client sends `Connection: close`
- Body reads are driven by `Content-Length`, looping `recv()` until the full request (headers + body) has arrived — correct even when a request spans multiple TCP segments
- Concurrent connections via `std::thread` (thread-per-connection model)
- Stage-based workflow (follows CodeCrafters stages for rapid iteration)
- Minimal external dependencies — STL, POSIX sockets, and zlib

---

## Known limitations

Being upfront about scope, since this is a learning/portfolio project rather than a production server:

- No `Expect: 100-continue` handling — compliant clients may briefly wait before sending large POST bodies
- No chunked transfer-encoding, HEAD/PUT/DELETE, or header folding
- Thread-per-connection concurrency (no thread pool / event loop) — fine for demonstration load, not tuned for high concurrency

---

## Quickstart

```bash
# clone
git clone https://github.com/Dyg-collab/codecrafters-http-server-cpp.git
cd codecrafters-http-server-cpp

# build (single-file)
g++ -std=c++17 -pthread src/main.cpp -O2 -o http-server -lz

# run (serve files from /tmp/http-files)
mkdir -p /tmp/http-files
./http-server --directory /tmp/http-files
```

Server listens on port `4221`.

### Try it

```bash
curl http://localhost:4221/
curl http://localhost:4221/echo/hello
curl -H "Accept-Encoding: gzip" http://localhost:4221/echo/hello --output -  | gunzip
curl -X POST --data "hello file" http://localhost:4221/files/test.txt
curl http://localhost:4221/files/test.txt
```

---

## License

MIT
