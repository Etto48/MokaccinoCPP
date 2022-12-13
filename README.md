# Mokaccino

[![Linux](https://github.com/Etto48/MokaccinoCPP/actions/workflows/linux-cmake.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/linux-cmake.yml)
[![Windows](https://github.com/Etto48/MokaccinoCPP/actions/workflows/windows-cmake.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/windows-cmake.yml)
[![MacOS](https://github.com/Etto48/MokaccinoCPP/actions/workflows/macos-cmake.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/macos-cmake.yml)

Mokaccino is a simple P2P application written in C++ capable of creating unstructured networks for sending text messages and using VoIP

## Building

### Requirements

- [Cmake](https://cmake.org/)
- [Boost library](https://www.boost.org/)
- [PortAudio library](http://portaudio.com/)
- [Opus library](https://www.opus-codec.org/)

### Optional
- [PDCurses (windows)](https://pdcurses.org/)
- [NCurses (unix like)](https://invisible-island.net/ncurses/)

### Instructions

You will need both Boost, PortAudio and Opus compiled and installed where Cmake can find them

Then you can run the following commands inside the project directory

```shell
mkdir build
cmake -B build
cmake --build build [--config Release]
```

## Configuration

A configuration file may be provided in the [TOML](https://toml.io) file format

### Example file

```TOML
[network]
# the username you like
username = "username" 

# this will be used as UDP port
port = 23232 

# you will connect to this list of servers automatically
autoconnect = ["server1.com","server2.net:24242"]

[network.audio]
# if someone in this list requests a voice call, the request is automatically accepted
whitelist = ["peer1"]

# this can be set to "accept" (auto accept every request), "refuse" (auto refuse every not whitelisted connection), "prompt" same as not defined
default_action = "prompt"

[network.connection]
# if someone in this list requests connection, the request is automatically accepted
whitelist = ["peer1","peer2"]

# this can be set to "accept" (auto accept every request), "refuse" (auto refuse every not whitelisted connection), "prompt" same as not defined
default_action = "prompt"

[terminal]
# you will run these commands after startup
startup_commands = ["msg server1.com hello everybody","voice start server2.net"]

# the format in which the time will be displayed near each message
time_format = "[%H:%M:%S]"
```

## Network protocol

Mokaccino uses an ascii protocol over UDP (because UDP hole punching is easier than TCP)

### Format

- Every message reported below ends with `'\n'`
- Tokens are separated with one or more whitespaces
- When you want to include a space in a token you must escape it with `'\'`
- When you want to include a `'\'` in a token you must escape it with another one

### Connection

The connection is established with a 3 way handshake

1) A: `CONNECT <A's username>`
2) B: `HANDSHAKE <B's username>`
3) A: `CONNECTED <A's username>`

#### Direct connection

You can directly send a connect request to any peer you know the address and name

#### Indirect connection

You can ask a peer to create a connection for you with one of it's connected peers

We will call the middle peer server and the two peers A and B

A to SERVER: `REQUEST <B's username>`

Then if the server is connected to a user with the requested username

SERVER to B: `REQUESTED <A's username> <A's IP>:<A's port>`

Now B tries to connect to A with the **direct connection** method

If the server is not connected to a user with the requested username

SERVER to A: `FAIL <B's username>`

#### Disconnection

If a peer you are connected with sends you `DISCONNECT [reason]` you must consider it disconnected and stop sending messages to it, the reason can be omitted

### Ping

You can ping another peer you are connected with using `PING <number>`, where number can be any number that is representable in 16b, the other peer must respond with `PONG <number>` with the same number of the ping message

### Text messages

If you want to send a text message with a user you are connected to

A to B: `MSG <message> [additional data]`

Additional data may be provided but by default is ignored

### Voice calls

You can request a voice call with another peer with

A to B: `AUDIOSTART`

If the other peer accepts

B to A: `AUDIOACCEPT`

Or if it refuses

B to A: `AUDIOSTOP`

This message can be also sent at any time to end the call

The audio data will be sent with the following format

`AUDIO <data encoded base64>`

## Libraries used

- [Boost](https://www.boost.org/)
- [PortAudio](http://portaudio.com/)
- [Opus](https://www.opus-codec.org/)
- [Toml++](https://github.com/marzer/tomlplusplus)
- [Base64](https://github.com/joedf/base64.c)
- [PDCurses](https://pdcurses.org/)
- [NCurses](https://invisible-island.net/ncurses/)
