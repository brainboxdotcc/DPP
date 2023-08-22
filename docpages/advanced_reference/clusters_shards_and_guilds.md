\page clusters-shards-guilds Clusters, Shards and Guilds

D++ takes a three-tiered highly scalable approach to bots, with three levels known as Clusters, Shards and Guilds as documented below.

\dot
digraph "Clusters, Shards and Guilds" {
    node [colorscheme="blues9",fontname="helvetica"];
    subgraph Bot {
        node [style=filled, color=1];
        label = "Bot"
        "Bot" [shape=folder, label="Bot", bordercolor=black];
    };
    subgraph Processes {
        node [style=filled, color=2];
        label = "Processes"
        "Bot" -> "Process 1"
        "Bot" -> "Process 2"
        "Process 1" [shape=record, label="Process"];
        "Process 2" [shape=record, label="Process"];
    };
    subgraph Clusters {
        node [style=filled, color=3];
        label = "Clusters"
        "Process 1" -> "Cluster 1"
        "Process 2" -> "Cluster 2"
        "Cluster 1" [shape=record, label="Cluster"];
        "Cluster 2" [shape=record, label="Cluster"];
    };
    subgraph Shards {
        node [style=filled, color=4];
        label = "Shards"
        "Shard 1" [shape=record, label="Shard"];
        "Shard 2" [shape=record, label="Shard"];
        "Shard 3" [shape=record, label="Shard"];
        "Shard 4" [shape=record, label="Shard"];
        "Shard 5" [shape=record, label="Shard"];
        "Shard 6" [shape=record, label="Shard"];
        "Shard 7" [shape=record, label="Shard"];
        "Shard 8" [shape=record, label="Shard"];
        "Cluster 1" -> "Shard 1"
        "Cluster 1" -> "Shard 3"
        "Cluster 2" -> "Shard 2"
        "Cluster 2" -> "Shard 4"
        "Cluster 1" -> "Shard 5"
        "Cluster 1" -> "Shard 7"
        "Cluster 2" -> "Shard 6"
        "Cluster 2" -> "Shard 8"
    };
    subgraph Guilds {
        node [style=filled, color=5];
        label = "Guilds";
        "Guild 1" [shape=record, label="Guild"];
        "Guild 2" [shape=record, label="Guild"];
        "Guild 3" [shape=record, label="Guild"];
        "Guild 4" [shape=record, label="Guild"];
        "Guild 5" [shape=record, label="Guild"];
        "Guild 6" [shape=record, label="Guild"];
        "Guild 7" [shape=record, label="Guild"];
        "Guild 8" [shape=record, label="Guild"];
        "Guild 9" [shape=record, label="Guild"];
        "Guild 10" [shape=record, label="Guild"];
        "Guild 11" [shape=record, label="Guild"];
        "Guild 12" [shape=record, label="Guild"];
        "Guild 13" [shape=record, label="Guild"];
        "Guild 14" [shape=record, label="Guild"];
        "Guild 15" [shape=record, label="Guild"];
        "Guild 16" [shape=record, label="Guild"];
        "Shard 1" -> "Guild 1"
        "Shard 1" -> "Guild 5"
        "Shard 2" -> "Guild 2"
        "Shard 2" -> "Guild 6"
        "Shard 3" -> "Guild 3"
        "Shard 3" -> "Guild 7"
        "Shard 4" -> "Guild 4"
        "Shard 4" -> "Guild 8"
        "Shard 5" -> "Guild 9"
        "Shard 5" -> "Guild 11"
        "Shard 6" -> "Guild 10"
        "Shard 6" -> "Guild 12"
        "Shard 7" -> "Guild 13"
        "Shard 7" -> "Guild 15"
        "Shard 8" -> "Guild 14"
        "Shard 8" -> "Guild 16"
    };
}
\enddot

## Clusters

A bot may be made of one or more clusters. Each cluster maintains a queue of commands waiting to be sent to Discord, a queue of replies from Discord for all commands executed, and zero or more **shards**. Usually, each process has one cluster, but the D++ library does not enforce this as a restriction. Small bots will require just one cluster. Clusters will split the required number of shards equally across themselves. There is no communication between clusters unless you add some yourself, they all remain independent without any central "controller" process. This ensures that there is no single point of failure in the design. Whenever you instantiate the library, you generally instantiate a cluster:

```cpp
#include <dpp/dpp.h>

int main()
{
	/* This is a cluster */
	dpp::cluster bot("Token goes here");
}
```

## Shards

A cluster contains zero or more shards. Each shard maintains a persistent connection to Discord via a websocket, which receives all events the bot is made aware of, e.g. messages, channel edits, etc. Requests to the API on the other hand go out to Discord as separate HTTP requests.

Small bots will require only one shard and this is the default when you instantiate a cluster. The library will automatically determine and create the correct number of shards needed, if you do not configure it by hand. If you do want to specify a number of shards, you can specify this when creating a cluster:

```cpp
#include <dpp/dpp.h>

int main()
{
	/* This is a cluster */
	int total_shards = 10;
	dpp::cluster bot("Token goes here", dpp::i_default_intents, total_shards);
}
```

Remember that if there are multiple clusters, the number of shards you request will be split equally across these clusters!

@note To spawn multiple clusters, you can specify this as the 4th and 5th parameter of the dpp::cluster constructor. You must do this, if you want this functionality. The library will not create additional clusters for you, as what you require is dependent upon your system specifications. It is your responsibility to somehow get the cluster id and total clusters into the process, e.g. via a command line argument. An example of this is shown below based on the cluster setup code of **TriviaBot**:
```cpp
#include <dpp/dpp.h>
#include <iostream>
#include <stdlib.h>
#include <getopt.h>
#include <string>

int main(int argc, char** argv)
{
	int total_shards = 64;
	int index;
	char arg;
	bool clusters_defined = false;
	uint32_t clusterid = 0;
	uint32_t maxclusters = 1;

	/* Parse command line parameters using getopt() */
	struct option longopts[] =
	{
		{ "clusterid",   required_argument,  NULL,  'c' },
		{ "maxclusters", required_argument,  NULL,  'm' },
		{ 0, 0, 0, 0 }
	};
	opterr = 0;
	while ((arg = getopt_long_only(argc, argv, "", longopts, &index)) != -1) {
		switch (arg) {
			case 'c':
				clusterid = std::stoul(optarg);
				clusters_defined = true;
			break;
			case 'm':
				maxclusters = std::stoul(optarg);
			break;
			default:
				std::cerr << "Unknown parameter '" << argv[optind - 1] << "'\n";
				exit(1);
			break;
		}
	}

	if (clusters_defined && maxclusters == 0) {
		std::cerr << "ERROR: You have defined a cluster id with -clusterid but no cluster count with -maxclusters.\n";
		exit(2);
	}

	dpp::cluster bot("Token goes here", dpp::default_intents, total_shards, clusterid, maxclusters);
}
```

### Large Bot Sharding

Discord restricts how many shards you can connect to at any one time to one per five seconds, unless your bot is in at least 150,000 guilds. Once you reach 150,000 guilds, Discord allow your bot to connect to more guilds concurrently, and your number of shards must divide cleanly into this value. By default, at 150,000 guilds this concurrency value is 16 meaning D++ will attempt to connect 16 shards in parallel, then wait for all these to connect and then connect another 16, until all shards are connected. In practice, this means a large bot with many shards (read: hundreds!) will connect significantly faster after a full restart. **You do not need to manually configure large bot sharding and connection concurrency, the D++ library will handle this for you if you are able to use it**.


## Guilds

Guilds are what servers are known as to the Discord API. There can be up to **2500** of these per shard. Once you reach 2500 guilds on your bot, Discord force your bot to shard, the D++ library will automatically create additional shards to accommodate if not explicitly configured with a larger number. Discord *does not restrict sharding* to bots on 2500 guilds or above. You can shard at any size of bot, although it would be a waste of resources to do so unless it is required. 
