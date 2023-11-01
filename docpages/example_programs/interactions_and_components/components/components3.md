\page components3 Using Select Menu Components

This tutorial will cover creating two types of select menus: 
- A generic select menu with just text
- An auto-populated role select menu.

This first example demonstrates creating a select menu, receiving select menu clicks, and sending a response message.

\include{cpp} components3.cpp

This second example demonstrates:
- Creating a role select menu that is auto-populated by Discord
- Allowing users to select two options!

\note This type of select menu, along with other types (these types being: user, role, mentionable, and channel), always auto-fill. You never need to define the data in these types of select menus. All select menu types allow you to select multiple options.

\include{cpp} components3_rolemenu.cpp
