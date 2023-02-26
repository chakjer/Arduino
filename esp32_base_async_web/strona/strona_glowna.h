const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang=pl>

<head>
  <title>ESP32 </title>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <link rel="stylesheet" type="text/css" href="styles.css">

</head>

<body>

  <header id="header">
    <div class="innertube">
      <h1>ESP32 - %statusNazwaHosta%</h1>
    </div>
  </header>

  <div id="wrapper">

    <main>
      <div id="content">
        <div class="innertube">
          <h1>Status</h1>
          <table>
            <tr>
              <th class="lewa">Adres IP:</th>
              <th class="prawa">%statusIpAdres%</th>
            </tr>
            <tr>
              <th class="lewa">Nazwa sieci:</th>
              <th class="prawa">%statusNazwaSieci%</th>
            </tr>
            <tr>
              <th class="lewa">Czas uruchomienia:</th>
              <th class="prawa">%statusGodzina%</th>
            </tr>
          </table>
        </div>
        <div id="przyciski">
          <button onclick="reboot()">Reboot</button>
        </div>
      </div>
    </main>

    <nav id="nav">
      <div class="innertube">
        <dl>
          <dt>Menu</dt>
          <dd><a href="/">Strona główna</a></dd>
          <dd><a href="ustawienia.html">Ustawienia</a></dd>
          <dt>Narzędzia</dt>
          <dd><a href="webserial" target="_blank">Konsola</a></dd>
          <dd><a href="update" target="_blank">OTA</a></dd>
        </dl>
      </div>
    </nav>

  </div>

  <footer id="footer">
    <div class="innertube">
      <p> </p>
    </div>
  </footer>
  <script>
    function reboot() {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", "reboot", true);
      xhttp.send();
    }




  </script>

</body>

</html>)=====";

const char PAGE_USTAWIENIA[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang=pl>

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <title>ESP32 </title>
  <link rel="stylesheet" type="text/css" href="styles.css">

</head>

<body>

  <header id="header">
    <div class="innertube">
      <h1>ESP32 - Konfiguracja</h1>

    </div>
  </header>

  <div id="wrapper">

    <main>
      <div id="content">
        <div class="innertube">
          <h2>Konfiguracja</h2>
          <table>
            <tr>
              <th class="lewa">WiFi:</th>
            </tr>
            <tr>
              <th class="lewa">Nazwa sieci:</th>
              <th><input type="text" id="ustawieniaNazwaSieci"></th>
            </tr>
            <tr>
              <th class="lewa">Hasło sieci:</th>
              <th><input type="text" id="ustawieniaHasloSieci"></th>
            </tr>
            <tr>
              <th class="lewa">Nazwa hosta:</th>
              <th><input type="text" id="ustawieniaNazwaHosta"></th>
            </tr>
            <tr>
              <th class="lewa">NTP:</th>
            </tr>
            <tr>
              <th class="lewa">Adres serwera:</th>
              <th><input type="text" id="ustawieniaNTPSerwer"></th>
            </tr>
            <tr>
              <th class="lewa">Strefa czasowa:</th>
              <th><input type="range" min="-12" max="12" id="ustawieniaNTPStrefa" class="slider1"></th>
              <th id="ustawieniaNTPStrefaCyfra"> </th>
            </tr>
            <tr>
              <th class="lewa">Korekta czasu letniego:</th>
              <th class="prawa"><label class="switch"><input type="checkbox" id="ustawieniaNTPCzasLetni"><span
                    class="slider"></span></th>
            </tr>
            <tr>
              <th class="lewa">RTC:</th>
            </tr>
            <tr>
              <th class="lewa">Stan:</th>
              <th class="prawa"><label class="switch"><input type="checkbox" id="ustawieniaRTCStan"><span
                    class="slider"></span></th>
            </tr>
            <tr>
              <th class="lewa">MQTT:</th>
            </tr>
            <tr>
              <th class="lewa">Stan:</th>
              <th class="prawa"><label class="switch"><input type="checkbox" id="ustawieniaMQTTStan"
                    onclick="ukryjMQTT()"><span class="slider"></span></th>
            </tr>
          </table>
          <table id="mqttMenu">
            <tr>
              <th class="lewa">Adres serwera:</th>
              <th class="prawa"><input type="text" id="ustawieniaMQTTSerwer"></th>
            </tr>
            <tr>
              <th class="lewa">Port serwera:</th>
              <th class="prawa"><input type="text" id="ustawieniaMQTTPort"></th>
            </tr>
            <tr>
              <th class="lewa">Autoryzacja:</th>
              <th class="prawa"><label class="switch"><input type="checkbox" id="ustawieniaMQTTAutoryzacja"
                    onclick="ukryjAutoryzacja()"><span class="slider"></span></th>
            </tr>
          </table>

          <table id="autoryzacjaMenu">
            <tr>
              <th class="lewa">Użytkownik:</th>
              <th class="prawa"><input type="text" id="ustawieniaMQTTUzytkownik"></th>
            </tr>
            <tr>
              <th class="lewa">Hasło:</th>
              <th class="prawa"><input type="text" id="ustawieniaMQTTHaslo"></th>
            </tr>
            <tr>
              <th class="lewa">Nazwa:</th>
              <th class="prawa"><input type="text" id="ustawieniaMQTTNazwa"></th>
            </tr>
          </table>
          <div id="przyciski">
            <button onclick="zapiszUstawieniaSieci()">Zapisz</button>
          </div>
          <p id="status"></p>
        </div>
      </div>
    </main>

    <nav id="nav">
      <div class="innertube">
        <dl>
          <dt>Menu</dt>
          <dd><a href="/">Strona główna</a></dd>
          <dd><a href="ustawienia.html">Ustawienia</a></dd>
          <dt>Narzędzia</dt>
          <dd><a href="webserial" target="_blank">Konsola</a></dd>
          <dd><a href="update" target="_blank">OTA</a></dd>
        </dl>
      </div>
    </nav>

  </div>

  <footer id="footer">
    <div class="innertube">
      <p> </p>
    </div>
  </footer>
  <script>

    function ukryjAutoryzacja() {
      var stanMenuAutoryzacja = document.getElementById("ustawieniaMQTTAutoryzacja");
      var menuAutoryzacja = document.getElementById("autoryzacjaMenu");
      if (stanMenuAutoryzacja.checked == true) {
        menuAutoryzacja.style.display = "block";
      } else {
        menuAutoryzacja.style.display = "none";
      }
    }

    function ukryjMQTT() {
      var stanMenuMQTT = document.getElementById("ustawieniaMQTTStan");
      var menuMQTT = document.getElementById("mqttMenu");
      var menuAutoryzacja = document.getElementById("autoryzacjaMenu");

      if (stanMenuMQTT.checked == true) {
        menuMQTT.style.display = "block";
      } else {
        menuMQTT.style.display = "none";
        menuAutoryzacja.style.display = "none";
      }
    }

    function zapiszUstawieniaSieci() {
      var nazwaSieci = document.getElementById("ustawieniaNazwaSieci").value;
      var hasloSieci = document.getElementById("ustawieniaHasloSieci").value;
      var nazwaHosta = document.getElementById("ustawieniaNazwaHosta").value;
      var serwerNTP = document.getElementById("ustawieniaNTPSerwer").value;
      var serwerNTPStrefa = document.getElementById("ustawieniaNTPStrefa").value;
      if (document.getElementById("ustawieniaNTPCzasLetni").checked == true) {
        var serwerNTPCzasLetni = 1;
      } else {
        var serwerNTPCzasLetni = 0;
      }
      if (document.getElementById("ustawieniaRTCStan").checked == true) {
        var rtcStan = 1;
      } else {
        var rtcStan = 0;
      }
      if (document.getElementById("ustawieniaMQTTStan").checked == true) {
        var mqttStan = 1;
      } else {
        var mqttStan = 0;
      }
      var mqttSerwer = document.getElementById("ustawieniaMQTTSerwer").value;
      var mqttPort = document.getElementById("ustawieniaMQTTPort").value;
      if (document.getElementById("ustawieniaMQTTAutoryzacja").checked == true) {
        var mqttAutoryzacja = 1;
      } else {
        var mqttAutoryzacja = 0;
      }
      var mqttUzytkownik = document.getElementById("ustawieniaMQTTUzytkownik").value;
      var mqttHaslo = document.getElementById("ustawieniaMQTTHaslo").value;
      var mqttNazwa = document.getElementById("ustawieniaMQTTNazwa").value;


      window.location.href = "/zapiszUstawieniaSieci?nazwaSieci=" + nazwaSieci + "&hasloSieci=" + hasloSieci + "&nazwaHosta=" + nazwaHosta + "&serwerNTP=" + serwerNTP + "&serwerNTPStrefa=" + serwerNTPStrefa +
        "&serwerNTPCzasLetni=" + serwerNTPCzasLetni + "&rtcStan=" + rtcStan + "&mqttStan=" + mqttStan + "&mqttSerwer=" + mqttSerwer + "&mqttPort=" + mqttPort + "&mqttAutoryzacja=" + mqttAutoryzacja + "&mqttUzytkownik=" + mqttUzytkownik +
        "&mqttHaslo=" + mqttHaslo + "&mqttNazwa=" + mqttNazwa;
      document.getElementById("status").innerHTML = "Zapisano";
    }


    const aktualizacjaStrefy = function () {
      var strefa = document.getElementById("ustawieniaNTPStrefa").value;
      document.getElementById("ustawieniaNTPStrefaCyfra").innerHTML = strefa;
    }


    document.querySelector('#ustawieniaNTPStrefa').oninput = aktualizacjaStrefy;

    function pobierzUstawieniaSieci() {
      var menuMQTT = document.getElementById("mqttMenu");
      var menuAutoryzacja = document.getElementById("autoryzacjaMenu");

      document.getElementById("ustawieniaNazwaSieci").value = "%ustawieniaNazwaSieci%";
      document.getElementById("ustawieniaHasloSieci").value = "%ustawieniaHasloSieci%";
      document.getElementById("ustawieniaNazwaHosta").value = "%ustawieniaNazwaHosta%";
      document.getElementById("ustawieniaNTPSerwer").value = "%ustawieniaNTPSerwer%";
      document.getElementById("ustawieniaNTPStrefa").value = "%ustawieniaNTPStrefa%";
      document.getElementById("ustawieniaNTPStrefaCyfra").innerHTML = "%ustawieniaNTPStrefa%";
      if ("%ustawieniaNTPCzasLetni%" == "1") {
        document.getElementById("ustawieniaNTPCzasLetni").checked = true;
      } else {
        document.getElementById("ustawieniaNTPCzasLetni").checked = false;
      }
      if ("%ustawieniaRTCStan%" == "1") {
        document.getElementById("ustawieniaRTCStan").checked = true;
      } else {
        document.getElementById("ustawieniaRTCStan").checked = false;
      }
      if ("%ustawieniaMQTTStan%" == "1") {
        document.getElementById("ustawieniaMQTTStan").checked = true;
        menuMQTT.style.display = "block";
      } else {
        document.getElementById("ustawieniaMQTTStan").checked = false;
        menuMQTT.style.display = "none";
      }
      document.getElementById("ustawieniaMQTTSerwer").value = "%ustawieniaMQTTSerwer%";
      document.getElementById("ustawieniaMQTTPort").value = "%ustawieniaMQTTPort%";
      if ("%ustawieniaMQTTAutoryzacja%" == "1") {
        document.getElementById("ustawieniaMQTTAutoryzacja").checked = true;
        menuAutoryzacja.style.display = "block";
      } else {
        document.getElementById("ustawieniaMQTTAutoryzacja").checked = false;
        menuAutoryzacja.style.display = "none";
      }
      document.getElementById("ustawieniaMQTTUzytkownik").value = "%ustawieniaMQTTUzytkownik%";
      document.getElementById("ustawieniaMQTTHaslo").value = "%ustawieniaMQTTHaslo%";
      document.getElementById("ustawieniaMQTTNazwa").value = "%ustawieniaMQTTNazwa%";


    }
    window.onload = pobierzUstawieniaSieci();

  </script>



</body>

</html>)=====";

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
body {
  margin: 0;
  padding: 0;
  font-family: Sans-Serif;
  line-height: 1.5em;
}

#header {
  background: #ccc;
  height: 100px;
}

#header h1 {
  margin: 0;
  padding-top: 15px;
}

main {
  padding-bottom: 10010px;
  margin-bottom: -10000px;
  float: left;
  width: 100%;
}

#nav {
  padding-bottom: 10010px;
  margin-bottom: -10000px;
  float: left;
  width: 240px;
  margin-left: -100%;
  background: #eee;
}

#footer {
  clear: left;
  width: 100%;
  background: #ccc;
  text-align: center;
  padding: 4px 0;
}

#wrapper {
  overflow: hidden;
}

#content {
  margin-left: 240px;
  /* Same as 'nav' width */
}

.innertube {
  margin: 15px;
  /* Padding for content */
  margin-top: 0;
}

p {
  color: #555;
}

nav ul {
  list-style-type: none;
  margin: 0;
  padding: 0;
}

nav ul a {
  color: darkgreen;
  text-decoration: none;
}

/* moje definicje*/
.lewa {
  text-align: right;
  width: 220px;
}

.prawa {
  font-weight: normal;
  text-align: left;
 
}

#przyciski {
  padding-left: 185px;;
  text-align: left;
  vertical-align: center;
}


.switch {
  position: relative;
  top: 4px;
  display: inline-block;
  width: 30px;
  height: 17px;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  -webkit-transition: .4s;
  transition: .4s;
}

.slider:before {
  position: absolute;
  content: "";
  height: 13px;
  width: 13px;
  left: 2px;
  bottom: 2px;
  background-color: white;
  -webkit-transition: .4s;
  transition: .4s;
}

input:checked+.slider {
  background-color: #2196F3;
}

input:focus+.slider {
  box-shadow: 0 0 1px #2196F3;
}

input:checked+.slider:before {
  -webkit-transform: translateX(13px);
  -ms-transform: translateX(13px);
  transform: translateX(13px);
}


.slider1 {
  -webkit-appearance: none;
  width: 100%;
  height: 19px;
  background: #d3d3d3;
  outline: none;
  opacity: 0.7;
  -webkit-transition: .2s;
  transition: opacity .2s;
}

.slider1:hover {
  opacity: 1;
}

.slider1::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 15px;
  height: 15px;
  background: #2196F3;
  cursor: pointer;
}

.slider1::-moz-range-thumb {
  width: 15px;
  height: 15px;
  background: #2196F3;
  cursor: pointer;
}


/* Chrome, Safari, Edge, Opera */
input::-webkit-outer-spin-button,
input::-webkit-inner-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

/* Firefox */
input[type=number] {
  -moz-appearance: textfield;
}



​
)=====";
