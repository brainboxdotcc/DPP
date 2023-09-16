\page attach-file Attaching a file to a message

Attached files must be locally stored.

To attach a file to a message, you can upload a local image.

D++ has this helper function to read a file: dpp::utility::read_file.

An example program:

\include{cpp} attachments1.cpp

Attachments via an url aren't possible. But there's a workaround for. You can download the file and then attach it to the message.

To make requests, D++ also has a helper function: dpp::cluster::request.

The following example program shows how to request a file and attach it to a message.

\include{cpp} attachments2.cpp

Here's another example of how to add a local image to an embed.

Upload the image in the same message as the embed and then reference it in the embed.

\include{cpp} attachments3.cpp
