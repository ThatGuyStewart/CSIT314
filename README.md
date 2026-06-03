# CSIT314

## File locations and purpose ##

/release contains a working build for Windows x64, all required .dll files, and a readme.txt explaining program functionality.

/release/HTML contains all HTML, CSS, and Javascript, under associated folders listed in the technical document.

/release/cert contains the self-signed certificate and key required for the SSL server.

/release/database contains schema.sql with the database schema and seed data.

/src contains all C++ source files. 

/src/vendor contains external C++ library headers.


## Compilation requirements ##

To compile the C++ application, the following external dependencies are required:
openssl
libpqxx

