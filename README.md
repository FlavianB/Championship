# Championship

This project enables its users to manage different tournaments and simulate winners for its tournaments. It also includes an email notification mechanism.

# Prerequisites for compiling:

libcurl: sudo apt-get install -y libcurl-dev

SQLite: sudo apt-get install -y libsqlite3-dev

# Compiling:

You need to open a terminal in the directory where Makefile is present and type the command 'make'. I used Makefile to make compiling easier and straight-forward, as multiple source files are present in the project.

The executables will be created in the 'build' directory and the database will be created wherever the executables are run.

# Running:

You will need to be in the 'build' directory. Run the server with './server' and the client with './client 127.0.0.1' (or any IP address as long as in the same network).

# About the database:

The database clears whenever the server is started. This can be easily modified, but was let as is for developing purposes.

# About emails:

The email address from which emails are sent and the authentification key for the Google SMTP are hardcoded in the app. These might not work on another computer. Also, the email address might expire on Feb 5th 2024 if not renewed.
