# Mokaccino

[![CodeQL](https://github.com/Etto48/MokaccinoCPP/actions/workflows/codeql.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/codeql.yml)

[![Linux](https://github.com/Etto48/MokaccinoCPP/actions/workflows/linux-cmake.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/linux-cmake.yml)
[![Windows](https://github.com/Etto48/MokaccinoCPP/actions/workflows/windows-cmake.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/windows-cmake.yml)
[![MacOS](https://github.com/Etto48/MokaccinoCPP/actions/workflows/macos-cmake.yml/badge.svg)](https://github.com/Etto48/MokaccinoCPP/actions/workflows/macos-cmake.yml)

Mokaccino is a simple P2P application written in C++ capable of creating unstructured networks for sending text messages, using VoIP and sending files, the protocol now supports authenticaticated connection and message encryption.

## Building

### Requirements

- [Cmake](https://cmake.org/)
- [Boost library](https://www.boost.org/)
- [PortAudio library](http://portaudio.com/)
- [Opus library](https://www.opus-codec.org/)
- [OpenSSL library](https://www.openssl.org/)

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

For more information about the installation of the dependencies see [windows-cmake.yml](.github/workflows/windows-cmake.yml), [linux-cmake.yml](.github/workflows/linux-cmake.yml) and [macos-cmake.yml](.github/workflows/macos-cmake.yml)

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

# this is the minimum volume to send the audio, to disable the voice activation detection set this to 0
threshold = 40

[network.connection]
# if someone in this list requests connection, the request is automatically accepted
whitelist = ["peer1","peer2"]

# this can be set to "accept" (auto accept every request), "refuse" (auto refuse every not whitelisted connection), "prompt" same as not defined
default_action = "prompt"

# if set to true every connection will be encrypted after the 3-way handshake
encrypt_by_default = true

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

1) A: `CONNECT <A's username> <A's nonce> <A's public key> <A's signature>`
2) B: `HANDSHAKE <B's username> <B's nonce> <A's nonce> <B's public key> <B's signature>`
3) A: `CONNECTED <B's nonce> <B's signature>` 

B must store A's public key for the next time and verify the signature received from A

A must store B's public key for the next time and verify the signature received from B

If a peer has saved a public key it must ignore every other key sent to him from the same user

#### Signature

The signature and verification algorithm uses ECDSA over the secp521r1 prime curve (you can set it to RSA with a CMake option) with SHA384 as message digest algorithm

#### Direct connection

You can directly send a connect request to any peer you know the address and name

#### Indirect connection

You can ask a peer to create a connection for you with one of it's connected peers

We will call the middle peer server and the two peers A and B

A to SERVER: `REQUEST <B's username>`

If the server knows the B's private key

SERVER to A: `KEY <B's username> <B's public key> <SERVER's signature>`

You must verify that the message is correctly signed from the server, otherwise there is a risk for MiM attacks

Then if the server is connected to a user with the requested username

SERVER to B: `REQUESTED <A's username> <A's IP>:<A's port> <A's public key> <SERVER's signature>`

You must verify that the message is correctly signed from the server, otherwise there is a risk for MiM attacks

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

### File transfers

Keep in mind that both `<file hash>` and `<data>` are encoded in Base64

You can send a file to a user sending

A to B: `FILEINIT <file hash> <file size> <file name>`

If B accepts it can send an ACK with sequence number 0

B to A: `FILEACK <file hash> 0`

A can send data to B with

A to B: `FILE <file hash> <sequence number> <data>`

`<sequence number>` is the index of the first byte of `<data>`

B can tell to A that it received the data until the byte `<n>` with

B to A: `FILEACK <file hash> <n+1>`

If `<n+1>` equals the size of the file you can consider the transaction finished

#### Notes about file transfers

`<data>` must always correspond to "CHUNK_SIZE" bytes or less (only if data is the last chunk, and in this case it must be exactly the size of the last chunk) if this condition is not met the packet will be discarded

You should send an ACK every time you have received "window_size" sequential chunks without ACKing them

You should send an ACK relative to the the last sequential chunk you received every time you receive a duplicate file packet, this can suggest that an ACK was lost

If you receive a duplicate ACK you should send the next chunk relative to the ACK

### End to end encryption

The algorithm used is ECDHE authenticated with ECDSA with hash SHA384 and the curve is secp521r1

The symmetric key cipher is AES256-GCM and the key is derived with HKDF (with hash SHA384) from the shared secret

#### Key exchange

The protocol used is ECDHE signed with ECDSA using the local key

A can request to B to encode the connection in the following way:

A must generate a new EC keypair (this must not be reused for other sessions or peers)

A to B: `CRYPTSTART <new public key> <signature with local key>`

Now B must check the signature and if accepts it must generate a new EC keypair and respond with:

B to A: `CRYPTACCEPT <new public key> <signature with local key>`

A to B: `CRYPTACK`

If B does not want to accept the encryption request or one of A or B wants to stop the encryption it must send `CRYPTSTOP`

#### Encrypted messages

After the encryption handshake has ended every message between A and B should be `C <encrypted message> <IV> <authentication tag>`

Every field is encoded base64, the symmetric key cipher is AES256-GCM

The initialization vector must be 8B once decoded

The tag must be 16B once decoded

## Libraries used

- [Boost](https://www.boost.org/)
- [PortAudio](http://portaudio.com/)
- [Opus](https://www.opus-codec.org/)
- [Toml++](https://github.com/marzer/tomlplusplus)
- [Base64](https://github.com/joedf/base64.c)
- [OpenSSL](https://www.openssl.org/)
- [PDCurses](https://pdcurses.org/)
- [NCurses](https://invisible-island.net/ncurses/)
