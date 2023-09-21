\page using-cache Using Cache

Sometimes you may need information that is not directly available in the event callback object.

To handle this DPP maintains a cache of commonly used data for you.

@note As of August 30th, 2022, Discord made Guild Members a privileged intent. You must have GUILD_MEMBERS intent enabled for your app from discord developers portal to be able to look for members in cache.

Below is an example showing how to get a user from the cache

\include{cpp} using_cache.cpp 

DPP caches more than just users, which you can get using the below-mentioned functions:
- `dpp::find_role()`
- `dpp::find_channel()`
- `dpp::find_emoji()`
- `dpp::find_guild()`
- `dpp::find_guild_member()`
