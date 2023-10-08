\page resolved-objects Using Resolved Objects

If your slash command accepts options like user, channel, or role you can get their value, as specified by the user in the command, from parameters. Though parameter gives you only the snowflake id of the passed value. 

If you need object of that snowflake, you can get that from the resolved set using its snowflake id.

Below is an example showing how to get a member, passed in command options, using resolved set.

\include{cpp} resolved_objects.cpp
