\page components_v2 Components V2

From March 2025 onwards Discord have released a **new way to handle components** in a Discord application/bot. The previous methods of working with components remain, and are accessible without
any changes in D++. If you want to use the new style of components you may do so, which gives far greater control over how message containing images, buttons, sections etc are formatted.

Components are attached to any message or interaction reply, via the dpp::message::add_component() or dpp::message::add_component_v2() function. You must also be sure to set the flag
dpp::m_using_components_v2 on the message to allow the new features.

When using components v2, the following limits apply which do not apply with components v1 or traditional embeds:

* Setting the message `content` or `embeds` will not be allowed (components v2 replaces the functionality)
* You can have a maximum of 10 top level components per message. The maximum number of nested components is 30.
* Audio files are not supported at present
* Text preview for text/plain files is not supported
* URLs will not have embeds generated for them
* The total length of the entire message, including all components within and the content of those components, is 4000 UTF-8 characters.

Here is a detailed example of how to create components v2 replies.

\warning Please note that where you would use add_component() previously, you **should** use add_component_v2() (on component or message objects). This is because the v2 version will not automatically add action rows, which is a limitation which has been removed in version 2 but is still required for version 1.

\include{cpp} components_v2.cpp

There are many new component types, for a complete list see the definition of dpp::component_type

If you run the example program above, you will be shown a message containing your components:

\image html components.gif
