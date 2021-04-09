# Clusters, Shards and Guilds

D++ takes a three-tiered highly scalable approach to bots, with three levels known as Clusters, Shards and Guilds as documented below.

## Clusters

A bot may be made of one or more clusters. Each cluster maintains a queue of commands waiting to be sent to Discord, a queue of replies from Discord for all commands executed, and zero or more **shards**.

Small bots will require just one cluster. Clusters may be started within separate processes if required (this is recommended) and will split the required number of shards equally across themselves. There is some level of communication between clusters, however they all remain independent without any central "controller" process.

## Shards

A cluster contains zero or more shards. Each shard maintains a persistent websocket connection to Discord which receives all events the bot is made aware of, e.g. messages, channel edits, etc. 

Small bots require only one shard. You may however create as many as you wish. Once you reach 2500 guilds, Discord require that your bot have at least one shard for every 2500 guilds. Any shard which would have over 2500 guilds on it will be disallowed connection.

## Guilds

Guilds are what servers are known as to the API. There can be up to **2500** of these per shard. Each of these is usually a distinct community and will contain a collection of guild members (users with a membership of this guild), channels and roles, emojis and various other pieces of data.
