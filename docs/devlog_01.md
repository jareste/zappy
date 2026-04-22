# Zappy Client - Devlog - 1

## Table of Contents
1. [There and Back Again (In a Network Sense)](#11---there-and-back-again-in-a-network-sense)
2. [A Small Step for Zappy, an Irrelevant Step for Humanity](#12---a-small-step-for-zappy-an-irrelevant-step-for-humanity)
3. [Hello From the Client Side](#13-hello-from-the-client-side)


<br>
<br>

# 1.1 - There and Back Again (In a Network Sense)
I could start this log by saying something like *New Project, New Me*, but that would be a blatant lie. Truth be told, this devlog finds me in a somewhat middle point of development, after a handful of months worth of hiatus from the 3 people team that set sail to build this *zappy* some forgotten amount of time ago. Back then, my main role was to build the GUI side of the project (my two partners were in charge of the Server and the Client), which I developed in Godot until the GUI-Server communication milestone was achieved. This means that on my side (and on the server's side), the Godot GUI could (and still can) connect to the initialized C server, receive an initial game session setup and build and display the game arena accordingly. All fine and dandy there (well, at some point I'm going to have to go back to the GUI and work my bottom off, but today is not that day), and the general assessment at this re-entry point is that we have a functional GUI, a working server and a (very) WIP client. This client was originally written in Java and had a considerable part of its implementation already done, but because I'm now taking over its development, some decisions are going to be made. The most critical: **we are going to rebuild an AI client for Zappy from scratch in my beloved C++**, using the Java work as reference, but essentially starting again. An interesting journey lies in front of us, then, one that will take me through some not-so-familiar roads, as I'm used to engine, systems, graphics and gameplay programming, not so much network client building. An opportunity, as they say, to change hats and reinforce my current knowledge of the subject and learn new things.

As always, right in that very moment before taking the dreaded first step, writing a roadmap sounds advisable, so we can start getting into motion with that. We can do it by analysing what is already done in the Java client and mixing it with the task's requirements, which gives us the following milestones:

0. **Build control and Bootstrapping**
	- Makefile with strict build paths and profile splitting
	- Zero-dependency baseline build on Linux dumps
	- A basic logger and error model (more on why later)
		- *At this point, the build pipeline should work in both debug and release profiles*
1.  **CLI compatibility**
	- Manage the required flags: -n team -p port -h host (defaulted to localhost)
	- Optional bonus flags (right now I don't know which, so, yeah). Some possibilities: --ws, --wss, --insecure, --protocol text|json
		- *At this point, the usage behavior should mirror the task expectation for required flags*
2. **Transport Layer**
	- TCP + TLS + WebSocket client implementation
	- Frame read/write, ping/pong, close handling
	- Backpressure-safe send queue.
		- *At this point, the client should connect and complete the JSON login flow in project mode, as well as handling WebSocket command/response stream without blocking or active wait*
3. **Protocol Layer**
	- JSON codec for login, cmd, response, message, event.
	- Strong validation and unknown-field tolerance.
	- Structured internal message model independent of transport.
		- *ATP, there should be a successful JSON handshake/login in a live run, as well as parsed and classified project response/event/message payloads*.
	- (OPTIONAL) Text serializer/parser for classic BIENVENUE/newline protocol.
	- (OPTIONAL) Runtime adapter boundary that does not affect JSON mode.
4. **Command pipeline**
	- In-flight cap hard-limited to 10.
	- Command queue with dedupe policies (for repeated voir and identical broadcast payloads)
	- Response-driven credit return
		- *ATP, client should never exceed 10 pending request and a stable command flow should be sustained for a 2-5 minute run*
5. **World and Player State Store**
	- Player state: level, inventory, life estimate, position/orientation confidence.
	- Vision decoding to local world model.
	- Team and connection counters
		- *ATP, state updates should match server responses over sustained runs*
6. AI State Machine v1 (Single Agent Survival)
	- Explicit SM:
		- BOOT
		- LOGIN
		- SCOUT
		- FEED
		- GATHER
		- ELEVATE_PREP
		- WAIT_ALLIES
		- INCANTING
		- COOLDOWN
	- Safety rules:
		- Starvation guard
		- cooldowns to avoid spam
		- no command storms
			- *ATP, a single agent should survive and progress with no deadlock loops* (Something NOT happening in the Java client)
7. **Team Coordination v1 (Broadcast)**
	- Broadcat parser/handler with directional convergence behavior.
	- Elevation call protocol for same-level grouping.
	- Timeout and fallback if regroup fails.
		- *ATP, a 3 agent test should reach at least one succesful coordinated elevation*
8. **Reproduction and Scaling**
	- fork and connect_nbr policies
	- Spawn policy thresholds based on map and food confidence
		- *ATP, there should be controlled client growth without immediate starvation collapse*
9. **OPTIONAL Compatibility Adapter (Text Protocol)**
	- Text serializer/parser for classic protocol
	- Newline transport wiring
	- Runtime switch between text and json protocol modules
		- *ATP, a text adapter should be in place and working in compatibility without breaking JSON project mode*
10. **Hardening and Documentation**
	- Full runbook for server +1, +3, +6, +10, +50 clients
	- Troubleshooting matrix
	- Performance and stability checklist
		- *ATP, there should be reproducible runs from a clean environment following the docs only*

That was intense, phew. Now, because I've been in the situation of waiting almost until the project reaches a production status to write test suites (bad), a from-the-beginning automated test-writing protocol should also be set in place. I'll base it on what I know, GTest, and target these suites:

- **Unit tests**
	- Protocol parsing/serialization fixtures
	- Queue credit logic and dedupe behavior
	- State Machine transitions and cooldown behavior
- **Integration tests**
	- JSON communication smoke runs (1, 3, 6, n clients)
	- Fault injection:
		- delayed responses
		- malformed JSON and malformed lines (for optional adapter)
		- forced disconnects
		- ping/pong bursts
- **Runtime assertions**
	- pending_count <= 10 always
	- no infinite tight loop without outgoing I/O or state change

> *If down the line the compilation times of the test suites start going out of control, I'll consider alternative tools like Catch2 or doctest*

Besides all of this, some general considerations regarding the technical baseline of the project:
- C++20
- Makefile
- GTest
- nlohmann/json
- OpenSSL for TLS

And, while we're at it, we can define a **Phase 1 objective**:
- New C++ client logs in and plays autonomously in JSON project mode
- No pending command overflow beyond 10
- Single-client behavior is stable for 5+ minutes
- 3-client run achieves at least one successful cooperative elevation
- Runbook allows repeatable execution in any environment

All of which will rest on these modules:
- `app/`
	- main entrypoint, CLI parsing, lifecycle
- `net/`
	- socket, TLS, WebSocket, reconnect/session wrappers
- `protocol/`
	- serializer/deserializer interfaces
	- json_protocol
	- text_protocol (OPTIONAL)
- `engine/`
	- command scheduler (max in-flight 10)
	- response correlator
	- state store (player/map/inventory/team)
- `ai/`
	- strategy layer
	- state machine
	- elevation coordinator
- `runtime/`
	- tick loop, timers, watchdogs, logging/tracing
- `tests/`
	- unit tests, protocol fixtures, integration harness

> *Also, let's keep transport, protocol and AI strategy independent via interfaces*

And we're ready to start. L E T ' S  G O ! ! !

<br>
<br>

# 1.2 - A Small Step for Zappy, an Irrelevant Step for Humanity
First thing's first: project setup. Nothing strange or fancy, just a very default-y Makefile with profiles, test rules and a bootstrapper with a basic logger. These two latter items being somewhat new to my usual project entryway, so let's talk about them. A concrete bootstrap step at the doors of `main()`, wired to a logger, is needed in this project due to its nature: **an autonomous networked client**.
- The **bootstrap** will act as a safety gate. If you're unfamiliar with the concept, just take it as an **initialization phase** that runs once when the client starts, before anything else happens, a kind of *startup ritual* where the runtime environment is prepared.
	- The command-line arguments are parsed
	- The transport layer is initialized (socket, TLS, WebSocket)
	- The configuration is laoded and validated
	- The command queue and State Machine are set up
	- There is an early return if anything fails (fail-fast approach)
- The **logger** is critical because there is no interactive debugging. I'm not going to be able to easily set a breakpoint or step through code when the client is running because of its asynchronous nature, while waiting for network responses or scheduling timed events. A text output is going to be needed to understand what happens during executions
	- Also, this will give the project a reproducible failure diagnosis. When something goes wrong along a run with several coordinated clients, the need for a complete trace of every decision, network event, and state transitions will set itself as a MUST. The logger will be our sword and our shield in that regard.
	- With several levels, verbosity will be controlled:
		- **Debug**: Every tiny decision (currently exploring tile 5, checking inventory, etc)
		- **Info**: Major state changes (login successful, elevation attempt started)
		- **Warn**: Recoverable problems (no allies nearby, retrying connection)
		- **Error**: Fatal problems (connection failed, protocol violated)
	- Also, this is **thread-safe**.

Anyways, the logger itself is not that big of a deal, quite rudimentary. The `bootstrapper`, on the other hand, could use some devlogging. As stated before, it is going to be a safety gate for the client initialization process, which will undergo the sequential steps stated above.

### 1.2.1 Argument Parsing
No need to say too much about this. If you're reading this, I'm sure you've parsed dozens of arguments in a C/C++ context. Run of the mill: **check count, capture content, validate data**. The usual first step in a program like this AI client is that kick-starting action that sets things in motion and quickly returns something that compiles, does *something*, tests the output pipelines (debug, info, all the layers in the `Logger`), and is in itself targetable for the automated test suite.

### 1.2.2. Transport Layer Initialization
Now we're talking. This second bootstrap step is going to need a triple attack combination:
1. **TCP layer (Foundation)**
	- Raw socket with connection/disconnection lifecycel
	- Non-blocking I/O
	- Read/Write buffers with proper backpressure handling
	- Graceful reconnection logic with exponential backoff
2. **TLS Layer (Security wrapper)**
	- OpenSSL integration wraps the TCP socket
	- Handshake negotiation (happens once, post-connect)
	- Endcrypted read/write that transforms plaintext buffers to/from ciphertext
	- Certificate validation (or `--insecure` bypass for testing)
	- Session reuse/resumption (ptional)
3. **WebSocket Layer (Protocol Framing)**
	- Sits on top of the TLS layer
	- Handles frame encoding/decoding (RFC 6455 compliance)
	- Implements ping/pong keepalive (prevents idle timeouts)
	- Manages connection upgrade (HTTP 101 Switching Protocols handshake)
	- Handles close frames gracefully

To build this, we're going to need `OpenSSL`, which means adding some flags to the build process in `Makefile`, and we could also use some external tools like a **Base64 encoder** (for frame masking) and a **SHA-1 hasher** (for the handshake). The structure planned is the following:
```
┌─────────────────────────────────────────────┐
│  WebSocket Frame Handler (public API)       │  Knows: frames, ping/pong, close
│  - readFrame() → FrameData                  │
│  - writeFrame(FrameData) → enqueue          │
│  - isPingExpected() → bool                  │
└────────────────────────┬────────────────────┘
                         │
┌─────────────────────────────────────────────┐
│  TLS Wrapper (transparent encryption)       │  Knows: handshake, ciphers
│  - tlsRead(buffer) → bytes_read             │
│  - tlsWrite(buffer) → bytes_written         │
│  - isHandshakeDone() → bool                 │
└────────────────────────┬────────────────────┘
                         │
┌─────────────────────────────────────────────┐
│  TCP Socket Manager (base I/O)              │  Knows: connect, FDs, buffers
│  - connect(host, port, tls_mode)            │
│  - read(buffer, max_bytes) → bytes_read     │
│  - write(buffer, bytes) → bytes_written     │
│  - close()                                  │
│  - isConnected() → bool                     │
└─────────────────────────────────────────────┘
```
> *The important key here: each layer exposes read/write ops and status queries, and lower layers don't know about upper layers.*

In a more specific way, and also more helpfully for figuring out how to write these three layers, the main idea is:
- TCP Baseline: low-level socket operations (creation, connection, binding, listening), with non-blocking flag set
- TLS Wrapping: certificate handling and context creation, handshake management, encryption
- Websocket Framing: encoding/decoding, ping/pong, frame handling, connection orchestration

Before starting, though, my research tells me that some decisions need to be made regarding how the data is going to be sent through the transport layer, what the ping interval is going to be, what reconnect strategy is going to be put in place, and what OpenSSL version is going to be handled. These are the initial decisions, which might change down the development line:
- **Deque based send queue**
	- **Non-blocking I/O safety**: `send()` can fail with `EAGAIN`/`EWOULDBLOCK` if the OS buffer is full. A queue allows a retry without losing data.
	- **Backpressure design**: A couple of milestones in a 10 in-flight commands maximum will be enforced. The queue will become the credit accounting mechanism, so that when a response arrives, the queue is popped and a credit is freed. If we start coding with this in mind we won't have to rework the architecture later (much more painfully).
	- **Flow stability**: without a queue, the AI layer would block. With a queue, AI keeps producing commands, and the network layer handles timing independently.
```
AI Layer                  Network Layer
   |                          |
   +---> enqueue_frame() ---> [SendQueue: deque<Frame>]
                               |
                         tick() / flush()
                               |
                         WebSocket::write()
                               |
                         TLS::write()
                               |
                         TCP::send()
```
> The queue will be **internal to WebsocketClient**. Each `tick()` makes a `send()` happen.

- **Ping Interval of 45 seconds** (configurable via class constant, maybe through config file)
	- **30-60 seconds** is RFC guidance, so 45 is a safe middle ground
	- **Sustain without spamming**, as many servers timeout idle connections at 60-120 seconds.
	- **Configurable** by setting it up as a static constant in `WebsocketClient`.
```cpp
class WebsocketClient {
    static constexpr int PING_INTERVAL_SECONDS = 45;
    int64_t last_ping_time = 0;  // in milliseconds or ticks
    
    void tick(int64_t now_ms) {
        if (now_ms - last_ping_time > PING_INTERVAL_SECONDS * 1000) {
            sendPing();
            last_ping_time = now_ms;
        }
    }
};
```

- **Fail hard and stop for reconnections**
	- Keeps things simple at this stage (and testable)
	- After achieving 5+ minutes stability with a single client, a more complex reconnection pipeline is relegated to a later milestone
	- This prevents an accidental "silent failure loop", i.e. a client silently retrying forever without logging
```cpp
Result WebsocketClient::connect(const std::string& host, int port) {
    Result tcp_res = tcp_socket_.connect(host, port);
    if (!tcp_res.ok()) {
        Logger::error("TCP connect failed: " + tcp_res.message);
        return tcp_res;  // Propagate to main, which exits
    }
    
    // TLS handshake, WebSocket upgrade...
    // If ANY step fails, return error and let main handle it
}
```
> It seems quite important to correctly, clearly and thoroughly log any failure at this point (errno, TLS error code, etc.).

- **OpenSSL 3.x accepting APIs from 1.1.x**
	- This is a compatibility shim that works on both old and new OpenSSL (and was already defined, so...)

Aaaand... At this point we should pump the breaks, take a deep breath and sit down for a while so that we can lay down how to correctly build a network client like the one we need. A 101, if you like, or if you're in the same out-of-your-lane situation as I am right now. Down the line, the needs and building objectives are clear, we'll need to have a main entry point, a bootstrapper and some type of runner class that manages the read/write communication between client and server (with sub managers regarding command sending, response processing, etc.). But between us and that point, all the net related code needs some careful logging so that all this process doesn't get lost in the turbulent torrents of the day to day battles.

<br>
<br>

## 1.3. Hello From The Client Side

### 1.3.1. TCPing Our Way Into the Server's Heart
Well, let's see. Following everything written before in this document, the first step in our net code should be the regular `TcpSocket` class, which will be the bottom-most layer in charge of the *basic* state and status tracking, as well as the low-level actions like connections, polls, state checks, read/write operations and error status retrieval. This is the layer in which the `TcpState` and `NetStatus` are enumerated and tracked, so we need enum classes in that regard, some way of organizing the possible range of both issues.
- For the `TcpState`, we'll need entry states for `Disconnected`, `Connecting`, `Connected`, `Closed`, all the possible states in which a TCP connection can be in.
- For the `NetStatus`, what we need is a way of classifying the state in which an established connection is in, which can be roughly collected in `Ok`, `WouldBlock`, `Connecting`, `ConnectionClosed`, `Timeout`, `InvalidState` and `NetworkError`.

Alongside this, we'll also need a way to store the result of an I/O operation, a small data package that stores the net status, the message written or read, its raw byte size and the possibility of an error state. Altogether, we can base this on a simple struct:
```cpp
struct IoResult {
	NetStatus	status = NetStatus::Ok;
	std::size_t	bytes = 0;
	std::string	message;
	int			sysErrno = 0;
};
```

Pretty straightforward up until this point. The next thing is functionality. What does the `TcpSocket` need to manage in concrete actions?
- Create and start a socket connection and set it to non-blocking
- Finalize connection states and set possible last errors
- Connect through the socket and track its state (connected, open)
- Poll the opened connection
- Read and write to the socket
- Store and track the socket fd

All in all, the `TcpSocket` could look like this:
```cpp
enum class TcpState {
	Disconnected,
	Connecting,
	Connected,
	Closed
};

enum class NetStatus {
	Ok,
	WouldBlock,
	Connecting,
	ConnectionClosed,
	Timeout,
	InvalidState,
	NetworkError
};

struct IoResult {
	NetStatus	status = NetStatus::Ok;
	std::size_t	bytes = 0;
	std::string	message;
	int			sysErrno = 0;
};

class TcpSocket {
	private:
		int			_fd = -1;
		TcpState	_state = TcpState::Disconnected;
		std::string	_host;
		int			_port = -1;
		int			_lastErrno = 0;
		std::string	_lastError;

	private:
		Result	createSocketAndStartConnect(const std::string& host, int port);
		Result	setNonBlocking(int socketFd);
		Result	finalizeConnectState();
		void	setLastError(int err, const std::string& msg);

	public:
		TcpSocket() = default;
		~TcpSocket();

		TcpSocket(const TcpSocket&) = delete;
		TcpSocket& operator=(const TcpSocket&) = delete;

		Result	connectTo(const std::string& host, int port);
		Result	pollConnect(int timeoutMs);
		void	close();

		bool	isConnected() const;
		bool	isConnecting() const;
		bool	isOpen() const;

		int			fd() const;
		TcpState	state() const;

		IoResult	readSome(std::vector<std::uint8_t>& out, std::size_t maxBytes);
		IoResult	writeSome(const std::vector<std::uint8_t>& data, std::size_t offset);

		std::string	lastErrorString() const;
		int			lastErrno() const;
};
```
Most of the net-related code is based on library functions and macros, and follows a standard setup. What's most important here and constitutes the core of the class is the socket creation and connection, which is laid out like this:
```cpp
Result TcpSocket::createSocketAndStartConnect(const std::string& host, int port) {
	addrinfo hints{};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addrinfo* head = nullptr;
	const std::string portStr = std::to_string(port);

	const int gai = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &head);
	if (gai != 0) {
		setLastError(0, std::string("getaddrinfo failed: ") + ::gai_strerror(gai));
		return Result::failure(ErrorCode::NetworkError, _lastError);
	}

	Result finalRes = Result::failure(ErrorCode::NetworkError, "No valid address for connection");

	for (addrinfo* ai = head; ai != nullptr; ai = ai->ai_next) {
		const int fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			setLastError(errno, std::string("socket() failed: ") + std::strerror(errno));
			continue;
		}

		const Result nbRes = setNonBlocking(fd);
		if (!nbRes.ok()) {
			::close(fd);
			continue;
		}

		const int rc = ::connect(fd, ai->ai_addr, ai->ai_addrlen);
		if (rc == 0) {
			_fd = fd;
			_state = TcpState::Connected;
			_lastErrno = 0;
			_lastError.clear();
			finalRes = Result::success();
			break;
		}

		const int err = errno;
		if (err == EINPROGRESS || err == EWOULDBLOCK) {
			_fd = fd;
			_state = TcpState::Connecting;
			_lastErrno = 0;
			_lastError.clear();
			finalRes = Result::success();
			break;
		}

		setLastError(err, std::string("connect() failed: ") + std::strerror(err));
		::close(fd);
		finalRes = Result::failure(ErrorCode::NetworkError, _lastError);
	}

	::freeaddrinfo(head);
	return finalRes;
}
```

Now, for some details...

#### 1.3.1.1 What Actually Happens During Non-Blocking Connect
If we zoom in on the sequence above, the order matters more than anything:

1. `getaddrinfo()` resolves host and port into candidate socket addresses.
2. For each candidate:
	- Create fd with `socket()`.
	- Set fd non-blocking via `fcntl()`.
	- Call `connect()` once.
3. `connect()` can return in 3 meaningful ways:
	- `0`: connection established immediately.
	- `-1` + `EINPROGRESS`/`EWOULDBLOCK`: expected for non-blocking mode, connection is in progress.
	- `-1` + other errno: hard failure for that candidate.
4. If connection is in progress, `pollConnect()` is then responsible for final confirmation (`poll()` + `getsockopt(SO_ERROR)`).

The key point is that in non-blocking mode, `connect()` is not the finish line. It is the start signal.

Also, a small language detail worth keeping in mind: calls like `::socket`, `::connect`, `::close` explicitly target global POSIX functions. This avoids ambiguity with class methods (for example, `TcpSocket::close()` vs global `::close(int)`).

### 1.3.1.2 Read/Write Contract
The `IoResult` shape is a tiny but very important design decision. It gives all higher layers the same contract:

- `status`: what happened (`Ok`, `WouldBlock`, `ConnectionClosed`, etc.)
- `bytes`: how much was consumed/produced
- `message`: human-readable context for logs
- `sysErrno`: raw error code for diagnostics

This contract lets TLS and WebSocket wrappers stay deterministic, and avoid guessing, hidden side effects and silent loops:

- If lower layer says `WouldBlock`, upper layer retries in next tick.
- If lower layer says `ConnectionClosed`, upper layer stops trying to push traffic.
- If lower layer says `NetworkError`, upper layer bubbles up and exits cleanly.

### 1.3.2. TLSing Our Secrets Into the Server's Hands
With a reliable TCP layer taking care of the low-level connection responsibilities, the second, in-between layer of the net sandwich should only add encryption and handshake management, all while avoiding new architecture chaos. The `TlsContext` (TLS as in `T`ransport `L`ayer `S`ecurity, as in the standard cryptographic protocol in computer networks, as in what comprises the core of `HTTPS`, as in the successor of `SSL`) has fewer responsibilities than the TCP lower layer, but they're quite specific:

1. Create SSL context and per-connection SSL session.
2. Bind SSL session to existing TCP fd.
3. Run handshake incrementally (non-blocking friendly).
4. Expose `readSome`/`writeSome`style operations that map SSL errors back to the already stablished `IoResult` model.

Architectural wise, a practical rule should be had in mind when adding a TLS layer on top of a working TCP level: **the objective is to build a somewhat filtered socket logic, not to have the TLS work as a second transport stack**. And in an even more *practical* mental approach, it could be said that the `TlsContext` class acts as a somewhat interface with the `openssl` library:

```cpp
class TlsContext {
public:
    ~TlsContext();

    TlsContext(const TlsContext&) = delete;
    TlsContext& operator=(const TlsContext&) = delete;

    static TlsContext& instance();

    Result initialize(bool insecureMode = false);
    SSL_CTX* getCtx() const;
    bool isInitialized() const;

private:
    TlsContext() = default;

    SSL_CTX* _ctx = nullptr;
    bool _initialized = false;
};
```

The core here is inside the `initialize()` function, which creates the SSL context (based on TLS 1.2+) and verifies the certificate for SSL (unless `insecureMode` is `true`). There's not that much of a secret here besides knowing what SSL related functions to call and in which order: I found out how out there, you can find out how in here (or out there too, you do you):

```cpp
Result TlsContext::initialize(bool insecureMode) {
	if (_initialized) {
		return Result::success();
	}

	_ctx = SSL_CTX_new(TLS_client_method());
	if (!_ctx) {
		const char* err = ERR_reason_error_string(ERR_get_error());
		std::string msg = std::string("SSL_CTX_new failed: ") + (err ? err : "unknown error");
		Logger::error(msg);
		return Result::failure(ErrorCode::NetworkError, msg);
	}

	if (!SSL_CTX_set_min_proto_version(_ctx, TLS1_2_VERSION)) {
		const char* err = ERR_reason_error_string(ERR_get_error());
		std::string msg = std::string("SSL_CTX_set_min_proto_version failed: ") + (err ? err : "unknown error");
		Logger::error(msg);
		SSL_CTX_free(_ctx);
		_ctx = nullptr;
		return Result::failure(ErrorCode::NetworkError, msg);
	}

	if (insecureMode) {
		SSL_CTX_set_verify(_ctx, SSL_VERIFY_NONE, insecure_mode_verify_callback);
		
		SSL_CTX_set_options(_ctx, SSL_OP_NO_QUERY_MTU);
		
		Logger::warn("TLS: Certificate verification disabled (insecure mode)");
	} else {
		SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER, nullptr);
		
		if (!SSL_CTX_set_default_verify_paths(_ctx)) {
			const char* err = ERR_reason_error_string(ERR_get_error());
			Logger::warn(std::string("Failed to load system CA store: ") + (err ? err : "unknown error"));
		}
	}

	_initialized = true;
	Logger::info("TLS context initialized successfully");
	return Result::success();
}
```

#### 1.3.2.1 What Insecure Basically Means
Quite simple, really. The TLS layer, built around SSL/HTTPS and certificate base, goes through certificate verification when building the secure context that is core to this level in the net sandwich (sorry, I just like that expression). In this sense, being *insecure* just means bypassing the verification and setting up the context in a *yeah, yeah, sure, go ahead* way. You write a fallback that sets the verification to, well, verified, and you keep on keeping on with your life.

```cpp
// Always return 1 (OK/accept) in insecure mode (hence the insecurity, huehue)
static int insecure_mode_verify_callback(int ok, X509_STORE_CTX* ctx) {
	(void)ok;
	(void)ctx;
	
	return 1;
}
```

### 1.3.3 WebSocket Layer: Protocol Framing and Session Semantics
With TLS in place, WebSocket becomes the final transport adapter for project JSON mode. This layer turns a secure byte stream into message-oriented communication, which is the whole reason the client can speak in `login`, `cmd`, `response`, `event` and `broadcast` payloads without caring about raw socket edges. If I had to describe this layer in one sentence, I would say this: **WebSocket is the protocol boundary that sits between encrypted bytes and application messages**. This means the layer is responsible for:

- Performing the HTTP upgrade handshake.
- Turning outgoing text into WebSocket frames.
- Turning incoming bytes back into complete frames.
- Keeping the connection alive with ping/pong.
- Handling graceful close and shutdown semantics.
- Respecting backpressure through an internal send queue.

> See the glossary in section 1.4 for the recurring transport terms used in this section.

This is also the point where the client stops thinking in terms of "send bytes" and starts thinking in terms of "send frames". That distinction matters a lot, because a WebSocket connection is not a plain stream anymore once the upgrade completes.

#### 1.3.3.1 Build Order
Because it might not be obvious: **the WebSocket layer should not be written first**. It only makes sense once the lower layers are already stable, and should be kept in mind as the last step in the process of building a network client like the one we've been fighting with. Just in case, and risking repetition, the sane order is:

1. `TcpSocket`: create a non-blocking TCP connection and make sure connect/poll works.
2. `TlsContext` and `SecureSocket`: wrap the TCP socket with TLS and make `read`/`write` work securely.
3. `FrameCodec`: encode and decode WebSocket frames independently of transport.
4. `WebsocketClient`: glue the handshake, frame codec, send queue and tick loop together.
5. Higher layers: command pipeline, JSON protocol, AI loop.

> *Notice that a `FrameCodec` layer has quietly crept up on us. Don't fret, it's just an in-between necessity for the byte-to-frame translations, in order to have the transport layer speak WebSocket at its top-most layer.*

> *The labeled "Higher layers" are just the client logic pieces, i.e. what the client does through its connection, i.e. how it is going to interact with the server, i.e. how it is going to play the Zappy game.*

#### 1.3.3.2 Handshake Responsibilities
The handshake is the moment where the client asks the server to stop speaking raw TLS data and start speaking WebSocket. The client sends an HTTP request like this:

```cpp
std::ostringstream oss;
oss << "GET / HTTP/1.1\r\n"
	<< "Host: " << _host << ":" << _port << "\r\n"
	<< "Upgrade: websocket\r\n"
	<< "Connection: Upgrade\r\n"
	<< "Sec-WebSocket-Key: " << ws_key << "\r\n"
	<< "Sec-WebSocket-Version: 13\r\n"
	<< "User-Agent: zappy-client\r\n"
	<< "\r\n";
```

Then the client waits for the server response and checks the minimum upgrade conditions:

- HTTP status `101 Switching Protocols`.
- `Sec-WebSocket-Accept` validation.
- End of headers marker `\r\n\r\n`.

In the current implementation, that work lives inside `WebsocketClient::performHandshake()` in [client_cpp/srcs/net/WebsocketClient.cpp](client_cpp/srcs/net/WebsocketClient.cpp). For more specific information and code, follow the link. I do not want to add even more clutter to this log.

#### 1.3.3.3 What `SecureSocket` Is Responsible For
`SecureSocket` is not the WebSocket layer itself. It is the TLS wrapper that WebSocket depends on. Its job is narrower and cleaner:

- Own the TCP socket object.
- Create and hold the TLS context.
- Start the TLS handshake.
- Expose encrypted `tlsRead()` and `tlsWrite()` calls.
- Translate OpenSSL errors into the same `IoResult` style used by the rest of the client.

This means `SecureSocket` cares about bytes, TLS status and connection validity, but not about WebSocket frame semantics. The class shape is intentionally small:

```cpp
class SecureSocket {
private:
	std::unique_ptr<TcpSocket> _tcp;
	SSL* _ssl = nullptr;
	bool _handshake_done = false;

	Result performHandshake();

public:
	Result connectTo(const std::string& host, int port, bool insecure = false);
	Result pollConnect(int timeoutMs);
	void close();

	IoResult tlsRead(std::vector<std::uint8_t>& out, std::size_t maxBytes);
	IoResult tlsWrite(const std::vector<std::uint8_t>& data, std::size_t offset);
};
```

The key idea is that `SecureSocket` gives the WebSocket layer a secure pipe, nothing more. That is a very good, necessary thing if you want anything else than going MAD debugging stuff, because it keeps the transport stack layered instead of turning it into a giant mixed-up blob.

#### 1.3.3.4 What `FrameCodec` Is Responsible For
If `SecureSocket` is the secure pipe, `FrameCodec` is the thing that knows how WebSocket messages are shaped. Take it as the translator, which has the job of understanding RFC 6455 framing:

- Build frame headers.
- Apply client masking.
- Decode payload length variants.
- Recover frames from a stream buffer.
- Create helper frames for text, ping, pong and close.

Because of its very specific, translation-related job, `FrameCodec` should stay completely unaware of sockets, TLS, AI state or command queues. Focused on its responsibilities and occupying its place in the net sandwich, it works on bytes only, encoding and decoding, bridging around the following relevant contract:

```cpp
class FrameCodec {
	public:
		static Result encodeFrame(const WebSocketFrame& frame, std::vector<std::uint8_t>& out);
		static Result decodeFrame(const std::vector<std::uint8_t>& data, std::size_t& offset, WebSocketFrame& out);

		static WebSocketFrame createTextFrame(const std::string& text);
		static WebSocketFrame createPingFrame();
		static WebSocketFrame createPongFrame();
		static WebSocketFrame createCloseFrame(uint16_t code = 1000, const std::string& reason = "");
};
```

All of this below two very important helper functions that take care of the masking, based around a strict protocol: client frames are masked on the way out, server frames are unmasked on the way in only when needed. Here are the signatures:

```cpp
static std::vector<std::uint8_t> generateMaskingKey();
static void applyMask(std::vector<std::uint8_t>& data, const std::uint8_t* mask);
```

#### 1.3.3.5 Frame Flow in the Current Client
Once the handshake completes, the logic becomes a two-direction frame pipeline. Outbound, the process can be boiled down to: create a command string, convert it into a WebSocket frame, serialize it, enqueue, send, flush. Cleanly ordered:

1. AI or command layer produces a command string.
2. `WebsocketClient` turns it into a `WebSocketFrame`.
3. `FrameCodec::encodeFrame()` serializes and masks it.
4. The encoded frame is pushed to the send queue.
5. `tick()` flushes the queue through `SecureSocket::tlsWrite()`.

The inbound path is the inverse process: read the raw, encrypted bytes received, buffer the content, rebuild a frame, process the content. Again, ordered:

1. `WebsocketClient::tick()` reads encrypted bytes through `SecureSocket::tlsRead()`.
2. The raw bytes are appended to the read buffer.
3. `FrameCodec::decodeFrame()` extracts one complete frame at a time.
4. The resulting text payload is handed to the higher protocol layer.

The goal of this design is to keep sanity, as I think I've stressed a handful of times at this point. Every component of the structure (the net sandwich) is constrained to its specific job and responsibility, with the clearest bounds possible: transport stays transport, framing stays framing, and commands stay commands. Mixed logic MUST be avoided, both across classes and across layers, and I think that the current net code finely achieves that goal. (But you tell me!)

One last note in this regard: **the send queue is part of the WebSocket layer because this is the first place where the notion of a discrete message exists**. If the socket would block, the frame must not be lost. If the TLS layer only accepts part of the write, the frame must remain queued. If the queue were owned by the AI logic instead, the AI would start mixing strategy with transport retry policy, which is exactly the kind of coupling that makes network clients miserable to debug. In other words, **the send queue must be as close as possible to the point where its contents are actually being sent**, not where they are ordered, built, written or masked. Which in itself means that the building, writing and masking steps are isolated and bound to their own completion before a frame is truly prepared to be enqueued and subsequently sent.

Alongside this, in the current implementation, the queue is flushed in `WebsocketClient::flushSendQueue()` and the important rule is simple: **do not treat a command as sent until the complete encoded frame has actually been written**. In human terms: the send action is what signals the end of the pipeline and triggers its cleanup.

#### 1.3.3.6 Ping, Pong and Close
Once the connection is running, the layer also becomes responsible for connection health.

- `ping` is the client heartbeat.
- `pong` is the server response and liveness confirmation.
- `close` is the graceful shutdown path.

Those are not application commands. They are protocol maintenance frames, so they belong here and nowhere else. They are not related to connection setup, they have nothing to do with security and encryption, they are just tools to keep the open WebSocket connection alive. Their respective functions in the current code are exposed directly through helpers like:

```cpp
WebSocketFrame FrameCodec::createPingFrame();
WebSocketFrame FrameCodec::createPongFrame();
WebSocketFrame FrameCodec::createCloseFrame(uint16_t code, const std::string& reason);
```

Besides the specifics, checkable in the code, the idea here is to build correct frames for these specific messages. Think of them as very specific, non-game-related commands sent to the server, which target it, not the game logic it manages. The server just receives them, notices that it is the addressee, and acts accordingly, assuming the frames are correctly built and written.

#### 1.3.3.7 What The Layer Should Never Do
Just for the sake of underlining what this WebSocket layer is and does, and even more what it *isn't* and it *doesn't*, it should never:

- Decide AI behavior.
- Parse game strategy rules.
- Track map knowledge.
- Implement command scheduling policy.
- Guess application meaning from protocol payloads beyond framing.

It should only answer the question: can the client and server exchange framed messages reliably, securely and without blocking? From there, it serves as the bridge for I/O communication: how the frames trigger this or that behavior on the client and server side is someone else's job, and bringing that logic here would only muddy the gears.

In sum, a very important construction, because if this layer is solid, the rest of the client can assume a reliable message stream and focus on higher-level concerns like inventory, vision, leveling and team coordination, which means I can focus on the command management layer and start having a game-playing agent. If it is weak, every later bug becomes ambiguous because you cannot tell whether a failure came from the AI or from the transport stack. So yes, the WebSocket layer is "just transport", but it is also the point where the client starts behaving like a real networked agent instead of a socket placeholder demo that only enables connection without a true communication protocol.

#### 1.3.3.7 App Layer Addendum: Bootstrap, Runner, Command Sender
After transport exists, application structure should avoid the old giant-main trap. Well, this might be a me-problem disguised as generalization, because I was ending up with a progressively more monstrous main file that was becoming hard to read, manage and debug. Therefore, a clean split was necessary:

- `ClientBootstrap`: parse and validate arguments.
- `ClientRunner`: execute runtime flow and ticking.
- `CommandSender`: centralize command emission (`login`, `voir`, `inventaire`, `prend nourriture`, etc.).

This gives a practical separation of concerns:

- Bootstrap decides whether startup is valid.
- Runner decides when actions happen.
- Command sender decides how commands are serialized and queued.

### 1.3.4 End-to-End Runtime Timeline (Current JSON Path)
For the sake of having a concise logged entry about how the client is set up and works, this is the exact order a healthy run follows:

1. Parse CLI arguments.
2. Connect TCP (non-blocking).
3. Complete TLS handshake.
4. Complete WebSocket upgrade.
5. Receive optional initial `bienvenue` frame.
6. Send `login` payload.
7. Receive `welcome` or `error`.
8. Send initial `voir`.
9. Receive first `voir` reply.
10. If loop mode enabled:
	- periodic `voir`
	- periodic `inventaire`
	- conditional `prend nourriture` on low food
	- exit gracefully on `die` event

The runner is therefore a deterministic state progression even before introducing a formal state machine class.

### 1.3.5 If Building This From Scratch Again (Or For You, Dear Reader, Who Are Reading This to Learn How To Write Your Own Client)
If I had to restart from an empty directory tomorrow, this is the order I would follow, to minimize moving parts at each step and keep failures as local and explainable as possible:

1. Define shared result/error model (`Result`, `IoResult`, enums).
2. Implement `TcpSocket` with non-blocking connect + poll finalize.
3. Add `TlsContext` + `SecureSocket` over existing fd contract.
4. Add WebSocket handshake + frame codec + send queue.
5. Build smoke app that only connects and exchanges one known command.
6. Introduce bootstrap/runner separation.
7. Introduce command sender intermediary.
8. Add loop scheduling and survival heuristics.
9. Add explicit state machine once flow is stable and observable.

And while doing so, an important, constant self-reminder must be kept in sight: for this kind of client, logs are not decoration: they are the debugger. So this is my recommended minimum events to log:

- Connect attempts and selected endpoint
- TLS handshake start/success/failure
- WebSocket upgrade request + status validation
- Every state transition in runner
- Command enqueue/send events (at least debug level)
- Frame receive summary (type/cmd/status)
- Shutdown reason (normal exit, server close, protocol error, timeout)

### 1.4 - Glossary of Weird Net Related Words

This section collects the recurring terms used throughout the networking notes above, which were (and still are) alien terms to my game-dev little brain.

- `backpressure`: the condition where the writer cannot immediately accept more data, so the sender must queue or retry later instead of dropping work.
- `frame`: the smallest message unit in WebSocket; it wraps payload data with protocol metadata like opcode, masking and length.
- `handshake`: the connection negotiation step that upgrades the secure HTTP/TLS stream into a WebSocket session.
- `mask`: the client-side XOR transform required by the WebSocket protocol for outgoing frames.
- `payload`: the actual message content carried inside a frame after protocol headers are stripped away.
- `ping` / `pong`: protocol-level keepalive frames used to verify that the remote peer is still responsive.
- `secure pipe`: a useful mental model for `SecureSocket`; it is a TLS-protected byte channel, not a WebSocket-aware component.
- `send queue`: the internal buffer that stores encoded outbound frames until they can be written completely.
- `shutdown semantics`: the graceful close path that lets both peers stop using the connection without treating it as a crash.
- `WebSocket upgrade`: the HTTP `101 Switching Protocols` transition from raw HTTP/TLS transport into framed WebSocket messaging.

And some extra terms worth keeping in mind for this project:

- `non-blocking I/O`: socket operations that may return immediately with `WouldBlock` rather than stalling the whole client.
- `round trip`: one command sent and one matching response received.
- `state machine`: the explicit set of client modes that decides what the next legal action is.
- `in-flight command`: a command that was sent but has not yet been acknowledged by the server.
- `heartbeat`: the periodic liveness check that prevents idle disconnections.

> *Next episode: building a proper AI!*~