\page interactions-and-components Interactions And Components

Interactions are a unified way provided by Discord to handle \ref slashcommands "slash commands" and \ref components-menu "component interactions", such as \ref components "clicking a button". Slash commands are much better than message commands as they are not rate limited as much, meaning your bot can handle a lot more at once. They also contain a lot of data about a command, for example by making a command take a user as a parameter, \ref resolved-objects "the entire user object will be contained in the interaction" so you do not have to fetch it yourself, avoiding even more rate limits!

* \subpage slashcommands-menu
* \subpage user-only-messages
* \subpage resolved-objects
* \subpage components-menu
* \subpage modal-dialog-interactions
* \subpage context-menu
* \subpage thinking
