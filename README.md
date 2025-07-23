# PC128-WIFI-MODEM
Il modem è derivato dal progetto WiFi SIXFOUR per commodore 64, un modem virtuale WiFi basato su microcontrollore ESP 8266. Copyright (C) 2016 Paul Rickards <rickards@gmail.com>.
Ho modificato alcune parti hardware e software del progetto per farlo funzionare su PC 128 OLIVETTI PRODEST.
Il modem si connette alle porta 2 del joystick.
Accendi il computer e carica il software Modem da Rom o da cassetta.
Dopo la schermata di avvio battere INVIO, il modem avvierà la connessione all' HOTSPOT con le credenziali precedentemente inserite. A connessione avvenuta il software scaricherà eventuali aggiornamenti del firmware.
Dopodiché il modem sarà in modalità comandi AT (vedi guida COMANDI AT).
# CONNESSIONE BBS PC128
Digitando il comando ATDS9 il modem si connette alla pseudo BBS del PC129.
A connessione avvenuta è necessario accedere con login e password. Nel caso di prima registrazione procedere con il comando register user password ed attendere l' abilitazione.
Digitare HELP per l' elenco dei comandi.
# COMANDI AT
at$ssid=ssid_della_rete_wifi
at$pass=password_della_rete_wifi
at&w - scrittura credenziali in memoria
