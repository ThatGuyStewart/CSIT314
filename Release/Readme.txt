The program requires SQLpostre to be installed and running with default settings: host: "localhost", port: "5432", username: "postgres", password: "data"

Once the program is running it should display:

Database connection established.
Server starting at https://127.0.0.1:8080
Press Ctrl+C to stop and quit.

At which point, navigating to https://127.0.0.1:8080 with a web browser should display the website.

You will receive a warning that the site is unauthorised. This is due to the server hosting SSL with a self-signed certificate.
Click advanced -> continue to site anyway, or the equivalent for your particular browser.
