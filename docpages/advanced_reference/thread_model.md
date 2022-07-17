\page thread-model Thread Model

\dot
digraph "Thread Model" {
	graph [ranksep=1];
	node [colorscheme="blues9",fontname="helvetica"];
	"Discord Events" -> "Your Program"

	"Your Program" [style=filled, color=1, shape=rect]
	"Cluster" [style=filled, color=1, shape=rect]

	subgraph cluster_4 {
		style=filled;
		color=lightgrey;
		node [style=filled,color=2]
		"Your Program"
		"Cluster"
		label = "User Code";
	}

	subgraph cluster_0 {
		style=filled;
		color=lightgrey;
		node [style=filled,color=4]
		"Shard 1" [style=filled, color=4]
		"Shard 2"
		"Shard 3..."
		label = "Shards (Each is a thread, one per 2500 discord guilds)";
	}

	subgraph cluster_1 {
		style=filled
		color=lightgrey;
		node [style=filled,color=4]
        	"REST Requests"
		"Request In Queue 1"
		"Request In Queue 2"
		"Request In Queue 3..."
        	"Request Out Queue"
		label = "REST Requests (Each in queue, and the out queue, are threads)"
	}

	subgraph cluster_3 {
		style=filled
		color=lightgrey;
		node [style=filled,color=4]
        	"Discord Events" [style=filled,color=4]
		"User Callback Functions"
		label = "Events and Callbacks"
	}

	"Cluster" [shape=rect]
	"REST Requests" [shape=rect]
	"Request In Queue 1" [shape=rect]
	"Request In Queue 2" [shape=rect]
	"Request In Queue 3..." [shape=rect]
	"Shard 1" [shape=rect]
	"Shard 2" [shape=rect]
	"Shard 3..." [shape=rect]
	"Request Out Queue" [shape=rect]
	"Discord Events" [shape=rect]
	"User Callback Functions" [shape=rect]

	"Cluster" -> "REST Requests"
	"Shard 1" -> "Discord Events"
	"Shard 2" -> "Discord Events"
	"Shard 3..." -> "Discord Events"
	"Your Program" -> "Cluster"
	"Cluster" -> "Shard 1"
	"Cluster" -> "Shard 2"
	"Cluster" -> "Shard 3..."
	"REST Requests" -> "Request In Queue 1"
	"REST Requests" -> "Request In Queue 2"
	"REST Requests" -> "Request In Queue 3..."
	"Request In Queue 1" -> "Request Out Queue"
	"Request In Queue 2" -> "Request Out Queue"
	"Request In Queue 3..." -> "Request Out Queue"
	"Request Out Queue" -> "User Callback Functions"
	"User Callback Functions" -> "Your Program"
}
\enddot
