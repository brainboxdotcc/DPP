\page thread-model Thread Model

\dot
digraph "Thread Model" {
	graph [ranksep=1];
	node [colorscheme="blues9",fontname="helvetica"];
	"Your Program" [style=filled, color=1, shape=rect]
	"Cluster" [style=filled, color=1, shape=rect]

	subgraph cluster_4 {
		style=filled;
		color=lightgrey;
		node [style=filled,color=2]
		"Your Program"
		"Your Program" -> "Cluster"
		label = "(1)";
	}

	subgraph cluster_3 {
		style=filled;
		color=lightgrey;
		node [style=filled,color=2]
		"Cluster" -> "Event Loop"
		"Event Loop" -> "HTTP(S) requests"
		"Event Loop" -> "Voice Sessions"
		"Event Loop" -> "Shards"
		"Shards" -> "Websocket Events"
		label = "(2)";
	}


	subgraph cluster_0 {
		style=filled;
		color=lightgrey;
		node [style=filled,color=4]
		"Voice Sessions" -> "Websocket Events"
		"HTTP(S) requests" -> "Thread Pool (4..n threads)"
		"Websocket Events" -> "Thread Pool (4..n threads)"
		"Thread Pool (4..n threads)"
		label = "(3)";
	}

	"Cluster" [shape=rect]
	"Thread Pool (4..n threads)" [shape=rect]
	"HTTP(S) requests"  [shape=rect]
	"Shards"  [shape=rect]
	"Websocket Events"  [shape=rect]
	"Event Loop"  [shape=rect]
	"Voice Sessions"  [shape=rect]

}
\enddot

## Details

1. User Code - No assumptions are made about how your program threads, if at all.
2. The event loop manages all socket IO for the cluster. If you start the cluster with dpp::st_return this will run under its own thread, otherwise if you use dpp::st_wait it will run in the same thread as the caller of the dpp::cluster::start method.
The event loop will be either poll, epoll or kqueue based depending on your system capabilities. You should always start a cluster after forking, if your program forks, as various types of IO loop cannot be inherited by a forked process.
3. Set thread pool size via cluster constructor. Thread pool uses a priority queue and defaults in size to half the system concurrency value. Every callback or completed coroutine ends up executing here. The minimum concurrency of this pool is 4.
