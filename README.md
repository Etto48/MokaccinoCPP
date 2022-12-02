# Mokaccino

Mokaccino is a simple P2P application written in C++ capable of creating unstructured networks for sending text messages and using VoIP

## Building

### Requirements

- [Cmake](https://cmake.org/)
- [Boost library](https://www.boost.org/)
- [PortAudio library](http://portaudio.com/)

### Instructions

You will need both Boost and PortAudio compiled and installed where Cmake can find them

Then you can run the following commands
```shell
MokaccinoCPP> mkdir build
MokaccinoCPP> cmake -B build
MokaccinoCPP> cmake --build build [--config Release]
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

### Text messages

If you want to send a text message with a user you are connected to

A to B: `MSG <message> [additional data]`

Additional data may be provided but by default is ignored