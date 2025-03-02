\page shardless Shardless Cluster: Events via Webhooks

[TOC]

## What are shardless clusters?

D++ gives you two ways to handle Discord events: sharding via websockets (the default) and webhooks. Websocket shards connect to Discord, handle up to 2,500 servers each, and process events asynchronously via a REST API call. Webhooks, on the other hand, let Discord push events straight to your bot, skipping the extra API call and making responses instant. WebSockets are great for most bots, but webhooks are super efficient for massive bots, using fewer resources while handling tons of traffic. The recommended setup is to run Nginx or Apache as a reverse proxy in front of your D++ bot, ensuring security and scalability. Enabling webhooks is simple—just call a method in your bot to activate them. We’ll guide you through setting it up step by step.

A simple summary of this process and how it differs is shown below:

\dot
digraph DPP_Event_Delivery {
    rankdir=TB;
    node [shape=rect, style="filled, rounded", fontcolor=black, fontsize=10, fontname="Sans", width=3.0, height=0];

    subgraph cluster_websocket {
        label="Shard WebSockets Method";
        fontname="Sans";
        color=skyblue;
        style=filled;
        fillcolor=lightskyblue;

        websocket [label="WebSocket Shard connects to Discord\n- Each shard handles up to 2500 guilds", fillcolor=lightcoral];
        receive_event [label="Events received from Discord\n- Incoming events from multiple guilds arrive", fillcolor=salmon];
        dispatch_pool [label="Events dispatched to thread pool\n- Events are assigned to worker threads", fillcolor=lightgoldenrodyellow];
        user_process [label="User processes event asynchronously\n- Application logic handles event", fillcolor=yellowgreen];
        rest_api [label="User sends separate REST API request to\nDiscord - This confirms the event was\nprocessed.", fillcolor=mediumseagreen];

        websocket -> receive_event -> dispatch_pool -> user_process -> rest_api;
    }

    subgraph cluster_webhook {
        label="Webhook Method";
        fontname="Sans";
        color=skyblue;
        style=filled;
        fillcolor=lightskyblue;

        https_listener [label="HTTPS Listener waits for Discord webhooks\n- Server listens for incoming HTTP requests", fillcolor=lightcoral];
        webhook_event [label="Discord sends event via webhook\n- Event data sent in HTTP POST request", fillcolor=salmon];
        dispatch_webhook [label="Event dispatched to thread pool\n- Assigned to a worker thread", fillcolor=lightgoldenrodyellow];
        user_process_webhook [label="User processes the event\n- Application logic handles event", fillcolor=yellowgreen];
        respond_webhook [label="Response sent immediately in HTTPS\nresponse - No additional REST API request\nneeded", fillcolor=mediumseagreen];

        https_listener -> webhook_event -> dispatch_webhook -> user_process_webhook -> respond_webhook;
    }
}
\enddot

There are advantages and disadvantages to a shardless webhook bot, the main ones are:

<table>
<tr>
<th>Advantages</th>
<th>Disadvantages</td>
</tr>
<tr>
<td>
✅ Scales without restarts, any cluster can service any request <br>
✅ Can be highly scalable on demand using a load balancer <br>
✅ Can be scaled and proxied by cloudflare and other free services <br>
✅ No need to keep shards connected, saving resources <br>
</td>
<td>
❎ Only slash commands and other components interactions can be used as entry points to the bot. Other events do not fire. <br>
❎ Special consideration needs to be given to request/response flow. <br>
❎ Without shards you don't have cache, so must rely entirely on resolved data and API calls. <br>
</td>
</tr>
</table>

## Creating the Discord bot

The first step is actually the easiest; enabling websocket event support in D++ is as simple as one method call, and specific parameters when initialising the cluster.bot

Please see the example below:

\include{cpp} shardless.cpp

Note that you can get the public key from your application settings in the Discord Developer Portal. It is **not** your bot token!

\warning It is important that if your bot works via webhook events, you do not delay in responding. You must respond as fast as possible to the request, with an `event.reply()`. You can always acknowledge the response with `event.thinking()` and edit the interaction later, but this must be done in a separate flow, NOT within the slashcommand event. This is because when you are in the event handler you are directly building a HTTP response that is directly sent back to Discord in an established HTTP request. You can follow up with other API calls if you wish later on. Each request is placed into D++'s thread pool, so you don't have to worry too much about blocking for a little while but remember users expect snappy responses and Discord have a 3 second hard limit on replies!

## Setting Up a reverse proxy to route to your bot

Next, you should set up a reverse proxy using **Nginx** or **Apache** to forward requests from a public-facing HTTPS endpoint to a Discord bot listening on `localhost:3000`. Discord mandates that all webhook endpoints for interactions must be HTTPS with a valid non self-signed certificate.

We will use a free **Let's Encrypt** certificate for SSL on the proxy, while allowing the bot itself to run using plaintext or self-signed SSL.

### Prerequisites
- A **Linux server**
- **Nginx** or **Apache** installed
- A domain name (`example.com`) pointing to your server
- A Discord bot running on **port 3000**
- `certbot` installed for Let's Encrypt SSL

## Option 1: Setting Up Nginx as a Reverse Proxy

### Install Nginx
```sh
sudo apt update
sudo apt install nginx -y
```

### Configure Nginx
Create a new configuration file:
```sh
sudo nano /etc/nginx/sites-available/discord_bot
```

Paste the following for **plaintext (http)** bot communication:
```nginx
server {
    listen 80;
    server_name example.com;

    location / {
        proxy_pass http://localhost:3000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

If the bot uses **self-signed SSL**, modify `proxy_pass`:
```nginx
location / {
    proxy_pass https://localhost:3000;
    proxy_ssl_verify off;
}
```

Enable the site and restart Nginx:
```sh
sudo ln -s /etc/nginx/sites-available/discord_bot /etc/nginx/sites-enabled/
sudo systemctl restart nginx
```

### Enable Let's Encrypt SSL
```sh
sudo certbot --nginx -d example.com
```
Certbot will automatically update the Nginx configuration for HTTPS.

## Option 2: Setting Up Apache as a Reverse Proxy

### Install Apache and Modules
```sh
sudo apt update
sudo apt install apache2 -y
sudo a2enmod proxy proxy_http ssl
sudo systemctl restart apache2
```

### Configure Apache Virtual Host
```sh
sudo nano /etc/apache2/sites-available/discord_bot.conf
```

Paste the following for **plaintext (http)** bot communication:
```apache
<VirtualHost *:80>
    ServerName example.com

    ProxyPass "/" "http://localhost:3000/"
    ProxyPassReverse "/" "http://localhost:3000/"
</VirtualHost>
```

If the bot uses **self-signed SSL**, update it as follows:
```apache
<VirtualHost *:80>
    ServerName example.com

    ProxyPass "/" "https://localhost:3000/"
    ProxyPassReverse "/" "https://localhost:3000/"
    SSLProxyEngine on
    SSLProxyVerify none
    SSLProxyCheckPeerCN off
    SSLProxyCheckPeerName off
</VirtualHost>
```

Enable the site and restart Apache:
```sh
sudo a2ensite discord_bot.conf
sudo systemctl restart apache2
```

### Enable Let's Encrypt SSL
```sh
sudo certbot --apache -d example.com
```

## Configuring Your Public URL in the Discord Developer Portal

Finally, to enable your Discord bot to receive interactions such as slash commands, you need to update Discord with the URL you set up above.

This involves configuring the **Interactions Endpoint URL** in your Discord application's settings.

### Steps to Configure the Interactions Endpoint URL

1. **Access the Discord Developer Portal**:
   - Navigate to the [Discord Developer Portal](https://discord.com/developers/applications).
   - Log in with your Discord account.

2. **Select Your Application**:
   - In the **Applications** section, click on your bot's application to open its settings.

3. **Navigate to the "General Information" Tab**:
   - On the left sidebar, select **General Information**.

4. **Set the Interactions Endpoint URL**:
   - Locate the **Interactions Endpoint URL** field.
   - Enter your public URL (e.g., `https://example.com/interactions`).
   - Ensure this URL points to the endpoint you've set up to handle Discord's interaction payloads.
   - Take note of the **public key** for use in your bot

5. **Save Changes**:
   - Scroll to the bottom of the page and click **Save Changes**.

\image html webhook-app.png

### Important Considerations

- **Public Accessibility**: Ensure that the URL you provide is publicly accessible over the internet. Discord needs to reach this endpoint to send interaction events.

- **SSL/TLS Encryption**: As outlined above in the section on setting up your reverse proxy, the URL must use HTTPS, meaning you need a valid SSL/TLS certificate for your domain. If you followed the guide properly, this should be working.

- **Endpoint Validation**: Upon setting the Interactions Endpoint URL, Discord will send a `PING` request to verify the endpoint. D++ will ensure your bot replies appropriately.

You can find even more information on Discord's official documentation on [Setting Up an Interaction Endpoint](https://discord.com/developers/docs/getting-started#setting-up-an-interaction-endpoint).

## Further reading/Related pages

* dpp::cluster::enable_webhook_server
* dpp::discord_webhook_server
* dpp::http_server
* dpp::http_server_request
* dpp::socket_listener

