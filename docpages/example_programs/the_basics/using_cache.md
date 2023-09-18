\page using-cache Using Cache

Sometimes you may need information that is not directly available in the event callback object.

To handle this DPP maintains a cache of commonly used data for you.

Below is an example showing how to get a user from the cache

\include{cpp} using_cache.cpp 

DPP caches more than just users, which you can get using the below-mentioned functions:
- `dpp::find_role()`
- `dpp::find_channel()`
- `dpp::find_emoji()`
- `dpp::find_guild()`
- `dpp::find_guild_member()`