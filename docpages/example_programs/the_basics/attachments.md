\page attach-file Attaching a File to a Message

To attach a local file to an message, you can use dpp::utility::read_file. It's a helper function from D++ that allows you to read the file's content and sent it to discord.

An example of this:

\include{cpp} attachments1.cpp

Attachments via URL aren't possible. But there's a workaround for this! You can download the file and then attach it to the message.

Amazingly, D++ also has a function for this! You can use dpp::cluster::request to make HTTP requests, allowing you to go ahead and pull the content from a URL.

The following example program shows how to request a file and attach it to a message:

\include{cpp} attachments2.cpp

Here's another example of how to add a local image to an embed.

Upload the image in the same message as the embed and then reference it in the embed.

\include{cpp} attachments3.cpp
