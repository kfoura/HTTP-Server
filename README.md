# HTTP-Server
## C++ HTTP Server

This project is a simple multithreaded HTTP server implemented in C++. It supports basic RESTful operations (GET, POST, DELETE) on a JSON-based database, using a thread pool for concurrent request handling.

### Features
- Handles HTTP GET, POST, and DELETE requests
- Serves static HTML and JSON responses
- Stores and manages data in a `database.json` file
- Logs all requests to `requests.json`
- Uses a thread pool for concurrent client handling
- Minimal dependencies (uses [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing)

### Endpoints
#### `GET /`
Returns a simple HTML welcome page.

#### `GET /database`
Returns the contents of `database.json` as JSON. If the file does not exist or is empty, returns an error message.

#### `POST /database`
Adds a new object to the database. The request body must be a JSON object (without an `id` field; IDs are auto-generated).

**Example request body:**
```json
{
	"name": "John Doe",
	"email": "john@example.com"
}
```

#### `DELETE /database`
Deletes an object from the database by `id`. The request body must be a JSON object containing the `id` field.

**Example request body:**
```json
{
	"id": 1
}
```

### Building and Running
#### Prerequisites
- g++ (with C++17 support)
- Linux environment
- [nlohmann/json](https://github.com/nlohmann/json) single-header file (`json.hpp` in project root)

#### Build
You can build the server using the provided VS Code task or manually:

```bash
g++ -g -std=c++17 -Wall -Wextra -pedantic-errors -Weffc++ -Wno-unused-parameter -fsanitize=undefined,address *.cpp -o server -pthread
```

#### Run

```bash
./server
```
The server listens on port 8080 by default.

### Files
- `server.cpp` — Main server implementation
- `threadpool.h` — Simple thread pool for concurrent request handling
- `json.hpp` — nlohmann/json single-header library
- `database.json` — JSON database (auto-created/updated)
- `requests.json` — Log of all HTTP requests

### Notes
- The server is for educational/demo purposes and is not production-ready.
- Error handling is basic; malformed requests may not be fully supported.
- Only one route (`/database`) is supported for POST and DELETE.

---
Feel free to extend the server with more features or endpoints!
