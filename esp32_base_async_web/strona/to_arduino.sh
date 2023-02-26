#!/bin/bash
cat << EOF > strona_glowna.h
const char PAGE_MAIN[] PROGMEM = R"=====(
EOF

cat ./index.html >> strona_glowna.h

cat << EOF >> strona_glowna.h
)=====";

const char PAGE_USTAWIENIA[] PROGMEM = R"=====(
EOF

cat ./ustawienia.html >>strona_glowna.h

cat << EOF >> strona_glowna.h
)=====";

const char PAGE_404[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang=pl>
    <head>
        <title>404</title>
    </head>
    <body>
        <center><h1>ERROR CODE 404</h1></center>
        <center><h1>Koniec Internetu</h1></center>
    </body>
</html>
)=====";

const char PAGE_CSS[] PROGMEM = R"=====(
EOF

cat ./styles.css >> strona_glowna.h

cat << EOF >> strona_glowna.h
)=====";
EOF