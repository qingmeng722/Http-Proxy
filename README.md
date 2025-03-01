# HTTP Caching Proxy

This project is Homework 2 for the course "Engineering Robust Server Software." It involves developing an HTTP proxy server in C++ that forwards client requests to origin servers while caching responses according to HTTP caching rules. The proxy supports GET, POST, and CONNECT methods, handles multiple concurrent requests using multiple threads, and logs all activities in a specified format.

## Overview

- **Purpose:**  
  Build an HTTP proxy that caches 200 OK responses for GET requests and serves them from the cache when appropriate. The proxy must also correctly forward POST and CONNECT requests, with CONNECT used for HTTPS tunneling.

- **Key Features:**  
  - **Caching:** Caches 200 OK GET responses using expiration and re-validation rules.  
  - **Multi-threading:** Handles multiple concurrent requests with a shared, synchronized cache.  
  - **Robust Logging:** Logs all events (incoming requests, cache status, interactions with origin servers, responses, errors) in a prescribed format.  
  - **Error Handling:** Responds with proper HTTP error codes (e.g., 400 for malformed requests, 502 for corrupted responses) and gracefully handles external failures.  
  - **Docker Support:** Provides Dockerfile and docker-compose.yml for easy deployment, including port mapping and log directory mounting.

## Functional Requirements

### HTTP Request Handling

- **GET Requests:**  
  - Cache responses with status 200 OK.  
  - Check cache and log whether the resource is:  
    - Not in cache  
    - In cache but expired (with expiration time logged)  
    - In cache but requiring validation  
    - In cache and valid

- **POST and CONNECT Requests:**  
  - Forward the request to the origin server.  
  - For CONNECT, handle the tunnel setup and log tunnel closure when done.

## Logging Requirements

The proxy must produce detailed logs for every request using the formats below. Each log entry should include a unique request identifier (ID), and all timestamps must be in UTC using the `asctime` format.

### 1. New Request Logging

When a new request is received, log the following entry:
ID: "REQUEST" from IPFROM @ TIME

- **ID:** A unique identifier assigned to the request.
- **REQUEST:** The HTTP request line (e.g., `GET /index.html HTTP/1.1`).
- **IPFROM:** The client's IP address.
- **TIME:** The time when the request was received.

### 2. Cache Status for GET Requests

For GET requests, log one of the following messages based on the cache status:

- **Not in Cache:**
ID: not in cache

- **In Cache but Expired:**
ID: in cache, but expired at EXPIREDTIME

  - **EXPIREDTIME:** The expiration time of the cached entry.

- **In Cache, Requires Validation:**
ID: in cache, requires validation

- **In Cache and Valid:**
ID: in cache, valid

### 3. Origin Server Interaction

When the proxy contacts the origin server:

- **Sending a Request:**
ID: Requesting "REQUEST" from SERVER

  - **SERVER:** The origin server's hostname.

- **Receiving a Response:**
ID: Received "RESPONSE" from SERVER

  - **RESPONSE:** The first line of the HTTP response from the server.

### 4. Cache Decision Logging

After receiving a response for a GET request, log one of the following based on the caching decision:

- **Not Cacheable:**
ID: not cacheable because REASON

  - **REASON:** Explanation for why the response cannot be cached.

- **Cached Successfully:**
ID: cached, expires at EXPIRES

  - **EXPIRES:** The time when the cached entry will expire.

- **Cached but Requires Re-validation:**
ID: cached, but requires re-validation

### 5. Client Response Logging

When the proxy responds to the client, log:
ID: Responding "RESPONSE"

  - **RESPONSE:** The HTTP response line sent to the client.

### 6. Tunnel Closure (for CONNECT Requests)

For CONNECT requests that establish a tunnel, log the tunnel closure:
ID: Tunnel closed

### 7. Additional Log Messages

Optional log messages should adhere to the following formats:
- **Notes:**
ID: NOTE MESSAGE

- **Warnings:**
ID: WARNING MESSAGE

- **Errors:**
ID: ERROR MESSAGE

If no specific request ID is available, use `(no-id)` as the identifier.

All log messages should be clear, concise, and help trace the full lifecycle of each request.

## Error Handling

- **Malformed Requests:** Respond with HTTP 400.  
- **Corrupted Responses:** If the origin server returns a corrupted response, reply with HTTP 502.  
- **Graceful Degradation:** The proxy must handle external failures gracefully while logging all pertinent error information.

## Build and Run Instructions

1. **Deploy with Docker:**  
 - Build the Docker image using the provided Dockerfile.  
 - Run the following command to start the proxy with Docker:
   ```bash
   docker-compose up
   ```
 - This will map port `12345` on the host to the proxy container and mount the `logs` directory to `/var/log/erss`.

2. **Configure Your Browser:**  
 Set your browser's proxy settings to use `localhost:12345` and start browsing to test the proxy functionality.
