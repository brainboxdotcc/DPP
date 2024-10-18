\page discord-application-command-file-upload Using File Parameters for Application Commands (Slash Commands)

The program below demonstrates how to use the 'file' type parameter to an application command (slash command).
You must first get the `file_id` via `std::get`, and then you can find the attachment details in the 'resolved'
section, `event.command.resolved`.

The file is uploaded to Discord's CDN so if you need it locally you should fetch the `.url` value, e.g. by using
something like dpp::cluster::request().

\include{cpp} upload_parameter.cpp
