\page voice-model Voice Model

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
		"ssl_client::write";
		
		"discord_client::one_second_timer" -> "websocket_client::write";
		"websocket_client::write" -> "ssl_client::write";
		"ssl_client::write" -> "Discord";
		
		label = "This is where we start sending\nwebsocket connections to Discord.";
	}
	
	subgraph cluster_3 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"ssl_client::read_loop";
		"Response from Discord?";
		"No";
		"HTTP/1.1 204 No Content...";
		"HTTP/1.1 101 Switching Protocols";
		
		"ssl_client::read_loop" -> "Response from Discord?";
		"Response from Discord?" -> "No";
		"Response from Discord?" -> "HTTP/1.1 204 No Content...";
		"Response from Discord?" -> "HTTP/1.1 101 Switching Protocols";
		"No" -> "ssl_client::read_loop";
		
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
		"discord_voice_client::thread_run" -> "ssl_client::read_loop";
		
		label = "Voice initalisation.\nThis will only fire when 'voice_server_update' AND 'voice_state_update' has fired.\nIf everything goes well, Discord should send back '101 Switching Protocals'.";
	}
	
	subgraph cluster_6 {
		style=filled;
		color=lightgrey;
		node [style=filled, color=3, shape=rect]
		"discord_voice_client::handle_frame";
		
		"HTTP/1.1 101 Switching Protocols" -> "discord_voice_client::handle_frame";
		
		label = "Do the voice stuff.";
	}
    
	"Your bot" -> "guild::connect_member_voice";
	
	"discord_client::connect_voice" -> "message_queue";
	
	"message_queue" -> "discord_client::one_second_timer";
	"discord_client::one_second_timer" -> "message_queue";
	
	"voice_state_update::handle" -> "voiceconn::connect";
	"voice_server_update::handle" -> "voiceconn::connect";
}
\enddot
