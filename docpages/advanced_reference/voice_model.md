\page voice-model Voice Model

# High Level Summary

Discord's audio system consists of several layers and inter-related systems as shown in the flow chart below.

At the top level, connecting to a voice server requires the library to request the details of a voice server from Discord via the websocket for the shard where the
server is located. Performing this request will make Discord reply with a websocket URI and an ephemeral token (not the bot token) which are used to establish an
initial connection to this secondary websocket. Every connection to a voice channel creates a separate secondary websocket.

Once connected to this websocket, the library negotiates which protocols it supports and what encryption schemes to use. If you enabled DAVE (Discord's end-to-end
encryption scheme) this is negotiated first. An MLS (message layer security) group is joined or created. If you did not enable DAVE, this step is bypassed.

The secondary websocket then gives the library a shared encryption secret and the hostname of an RTP server, which is used to encrypt RTP packets using libssl.
This is stored for later.

The next step is to send an initial packet to the RTP server so that the library can detect the public IP where the bot is running. Once the RTP server replies,
the bot may tell the websocket what encryption protocols it is going to use to encrypt the RTP packet contents (leaving the RTP header somwhat intact).

The library is now in an initialised state and will accept method calls for `send_audio_raw()` and `send_audio_opus()`. If you send raw audio, it will first be
encoded as OPUS using libopus, and potentially repacketized to fit into UDP packets, with larger streams being split into multiple smaller packets that are scheduled
to be sent in the future.

If at this point DAVE is enabled, the contents of the OPUS encoded audio are encrypted using the AES 128 bit AEAD cipher and using the bot's MLS ratchet, derived
during the MLS negotiation which was carried out earlier. All valid participants in the voice channel may use their private key, and the public key derived from
their ratchets, to decrypt the OPUS audio.

Regardless of if DAVE is enabled or not, the OPUS stream (encrypted by DAVE, or "plaintext") is placed into an RTP packet, and then encrypted using the shared secret
given by the websocket, known only to you and Discord, using the xchacha20 poly1305 cipher.

The completed packet, potentially with two separate layers of encryption (one with a key only you and Discord know, and one with a key only you and participants in the
voice chat know!), plus opus encoded audio is sent on its way via UDP to the RTP server, where Discord promptly distribute it to all participants in the chat.

\image html audioframe.svg

After reading all this, go get a coffee or something, you deserve it! ☕

# Flow Diagram

\dot
digraph "Example Directory" {
	graph [ranksep=1];
	node [colorscheme="blues9", fontname="helvetica"];

	"Your bot" [style=filled, color=1, shape=rect]
	"Discord" [style=filled, color=1, shape=rect]
    
	subgraph cluster_0 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"guild::connect_member_voice";
		"discord_client::connect_voice";

		"guild::connect_member_voice" -> "discord_client::connect_voice";

		label = "This is the front-end of D++.\n'connect_voice' will now queue a JSON message.";
	}
	
	subgraph cluster_1 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=2, shape=rect]
		"message_queue";

		label = "This holds all our messages.\n'one_second_timer' reads this data";
	}
	
	subgraph cluster_2 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"discord_client::one_second_timer";
		"websocket_client::write";
		"ssl_connection::write";
		
		"discord_client::one_second_timer" -> "websocket_client::write";
		"websocket_client::write" -> "ssl_connection::write";
		"ssl_connection::write" -> "Discord";
		
		label = "This is where we start sending\nwebsocket connections to Discord.";
	}
	
	subgraph cluster_3 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"ssl_connection::read_loop";
		"Response from Discord?";
		"No";
		"HTTP/1.1 204 No Content...";
		"HTTP/1.1 101 Switching Protocols";
		
		"ssl_connection::read_loop" -> "Response from Discord?";
		"Response from Discord?" -> "No";
		"Response from Discord?" -> "HTTP/1.1 204 No Content...";
		"Response from Discord?" -> "HTTP/1.1 101 Switching Protocols";
		"No" -> "ssl_connection::read_loop";
		
		"Discord" -> "HTTP/1.1 204 No Content...";
		"Discord" -> "HTTP/1.1 101 Switching Protocols";
		
		label = "Now, we're waiting for a response from Discord.\nIf we receive 204, we'll start initiating voiceconn. However, if we receive 101, then we can do all the voice stuff.";
	}
	
	subgraph cluster_4 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"voice_state_update::handle";
		"voice_server_update::handle";
		
		"HTTP/1.1 204 No Content..." -> "voice_state_update::handle";
		"HTTP/1.1 204 No Content..." -> "voice_server_update::handle";
		
		label = "These events can fire in any order. Discord picks whatever it likes.";
	}
	
	subgraph cluster_5 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"voiceconn::connect";
		"new discord_voice_client"
		"websocket_client::connect"
		"discord_voice_client::run"
		"discord_voice_client::thread_run"
		
		"voiceconn::connect" -> "new discord_voice_client";
		"new discord_voice_client" -> "websocket_client::connect";
		"websocket_client::connect" -> "websocket_client::write";

		"voiceconn::connect" -> "discord_voice_client::run" [label="Once websocket_client has finished"];
		"discord_voice_client::run" -> "discord_voice_client::thread_run";
		"discord_voice_client::thread_run" -> "ssl_connection::read_loop";
		
		label = "Voice initalisation.\nThis will only fire when 'voice_server_update' AND 'voice_state_update' has fired.\nIf everything goes well, Discord should send back '101 Switching Protocals'.";
	}
	
	subgraph cluster_6 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"discord_voice_client::handle_frame";
		
		"HTTP/1.1 101 Switching Protocols" -> "discord_voice_client::handle_frame";
		
		label = "Do the voice stuff.";

        "discord_voice_client::handle_frame"-> "DAVE enabled";
        "discord_voice_client::handle_frame"-> "DAVE disabled";
        "DAVE disabled"->"discord_voice_client::send_audio_*()";
 "discord_voice_client::send_audio_*()" -> "Dave encryption on";
 "discord_voice_client::send_audio_*()" -> "Dave encryption off";
 "Dave encryption on" -> "AES AEAD encryption\nof OPUS stream\nusing ratchet";
 "AES AEAD encryption\nof OPUS stream\nusing ratchet" -> "XChaCha20-Poly1305 encryption";
 "Dave encryption off" -> "XChaCha20-Poly1305 encryption";
 "XChaCha20-Poly1305 encryption" -> "UDP sendto";
 "UDP sendto" -> "Discord RTP server";
 "DAVE enabled" -> "MLS send key package";
 "MLS send key package" -> "MLS receive external sender";
 "MLS receive external sender" -> "MLS proposals";
 "MLS proposals" -> "MLS Welcome";
 "MLS proposals" -> "MLS Commit";
 "MLS Commit" -> "DAVE begin transition";
 "MLS Welcome" -> "DAVE begin transition";
 "DAVE begin transition" -> "Dave execute transition";
 "Dave execute transition" -> "discord_voice_client::send_audio_*()";
	}
    
	"Your bot" -> "guild::connect_member_voice";
	
	"discord_client::connect_voice" -> "message_queue";
	
	"message_queue" -> "discord_client::one_second_timer";
	"discord_client::one_second_timer" -> "message_queue";
	
	"voice_state_update::handle" -> "voiceconn::connect";
	"voice_server_update::handle" -> "voiceconn::connect";
}
\enddot
