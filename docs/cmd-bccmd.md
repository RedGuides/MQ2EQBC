---
tags:
  - command
---

# /bccmd

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/bccmd [option] | toggle <option> | set <option> {on|off}
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Meta control of the EQBC client, including connecting to the server, channels, help, status, and more.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `connect [server] [port] [password]` | Defaults to server 127.0.0.1 and port 2112. Unless you have unique network settings or are connecting to another PC you shouldn't have to change these. |
| `quit` | Disconnects from the server |
| `forceconnect [server] [port] [password]` | This differs from *connect* in that it works while already connected to a server. |
| `iniconnect [KeyName]` | Connect to a server by ini key name using information defined in your MQ2EQBC.ini file. Example:<br> ;2100 would be the keyname<br> &#91;2100&#93;<br> Server=10.0.0.1<br> Port=2100<br><br> ;2150 would be the keyname<br> &#91;2150&#93;<br> Server=10.0.0.1<br> Port=2150<br> Password=''mypassword'' |
| `help` | Shows a description of all commands and settings |
| `status` | Displays whether connected or not as well as current settings |
| `reconnect [on|off]` | Automatically connect at start. Also, will close the current connection and connect again |
| `names` | Shows a list of clients (characters) connected to the server |
| `version` | Shows plugin version |
| `colordump` | Shows all available color codes |
| `channels <channel list>` | Add channels to receive tells and commands from. |
| `stopreconnect` | Stop trying to reconnect for now |
| `set <parameter> {on|off} | toggle <parameter>` | These parameters are covered on the [plugin page](index.md#settings), please refer to that page for more information. |

## Examples

Auto reconnect at the start of each session
```eqcommand
/bccmd reconnect on
```

Sending commands to channels:
```eqcommand
/bccmd channels chatchan, commands
/bct chatchan hey there guys
/bct commands //bct chatchan My zone: ${Zone.ShortName}
```
