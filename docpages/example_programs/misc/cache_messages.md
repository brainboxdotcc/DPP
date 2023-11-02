\page caching-messages Caching Messages

By default D++ does not cache messages. The example program below demonstrates how to instantiate a custom cache using dpp::cache which will allow you to cache messages and query the cache for messages by ID.

This can be adjusted to cache any type derived from dpp::managed including types you define yourself.

\note This example will cache and hold onto messages forever! In a real world situation this would be bad. If you do use this, you should use the dpp::cache::remove() method periodically to remove stale items. This is left out of this example as a learning exercise to the reader. For further reading please see the documentation of dpp::cache.

\include{cpp} cache_messages.cpp
