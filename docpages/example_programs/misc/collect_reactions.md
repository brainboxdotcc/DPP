\page collecting-reactions Collecting Reactions

D++ comes with many useful helper classes, but amongst these is something called dpp::collector. Collector is a template which can be specialised to automatically collect objects of a predetermined type from events for a specific interval of time. Once this time period is up, or the class is otherwise signalled, a method is called with the complete set of collected objects.

In the example below, we will use it to collect all reactions on a message.

\include{cpp} collect_reactions.cpp

