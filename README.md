This repository contains a multiclient chat server and a an automated client.
The multiclient chat server is written in C and is for unix operating systems.
Chat_bot can take up one of two roles upon connecting to the server, a reader or a logger.
A reader bot does not participate in the chat but reads every message in the chat room and looks for banned word, once it finds any the bot alerts a logger bot.
A logger bot saves the reported message with a timestamp and a username of the sender in a file.
