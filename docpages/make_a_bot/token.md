\page creating-a-bot-application Creating a Bot Token

Before you start coding, you need to create and register your bot in the Discord developer portal. You can then add this bot to your Discord-server.

## Creating a new bot

To create a new application, take the steps as follows:

1. Sign in to the [Discord developer portal](https://discord.com/developers/applications) and click on "New Application" on the top right.
2. Next, enter a name for the application in the pop-up and press the "Create" button.
\image html create_application_confirm_popup.png
In this example we named it "D++ Test Bot".
3. Move on by click the "Bot" tab in the left-hand side of the screen. Now click the "Add Bot" button on the right and confirm that you want to add the bot to your application.

\image html create_application_add_bot.png

On the resulting screen, you’ll note a page with information regarding your new bot. You can edit your bot name, description, and avatar here if you want to. If you wish to read the message content from messages, you need to enable the message content intent in the "Privileged Gateway Intents" section.

\image html create_application_bot_overview.png

In this panel, you can get your bot token by clicking "Reset Token". A bot token looks like this: `OTAyOTMxODU1NTU1MzE3ODUw.YXlm0g.9oYCt-XHXVH_z9qAytzmVRzKWTg`

\warning **Do not share this token** with anybody! If you ever somehow compromise your current bot token or see your bot in danger, you can regenerate the token in the panel.

## Adding the bot to your server

Once you've created your bot in the discord developer portal, you may wonder:
> Where is my bot now, I can't see him on my server?!

That's because you've created a bot application, but it's not on any server right now. So, to invite the bot to your server, you must create an invitation URL.

1. go again into the [Applications page](https://discord.com/developers/applications) and click on your bot.
2. Go to the "OAuth2" tab and click on the subpage "URL Generator".
\image html create_application_navigate_to_url_generator.png
3. Select the `bot` scope. If your bot uses slash commands, also select `applications.commands`. You can read more about scopes and which you need for your application [here](https://discord.com/developers/docs/topics/oauth2#shared-resources-oauth2-scopes).
4. Choose the permissions required for your bot to function in the "Bot Permissions" section.
5. Copy and paste the resulting URL in your browser. Choose a server to invite the bot to, and click "Authorize".


\note For bots with elevated permissions, Discord enforces two-factor authentication on the bot owner's account when added to servers that have server-wide 2FA enabled.

## Troubleshooting

- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!

