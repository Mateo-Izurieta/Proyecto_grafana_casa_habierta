Este proyecto se encargara de recopilar los datos obtenidos por todos los proyectos del curso, y mostrarlos por una 
grafica a travez de grafana

Ese es el codigo utilizado para el proyecto para la casa habierta, en la version base, donde solo se usa de ejemplo un proyecto beta llamado
"sensor" que mide temperatura-Altura-humedad-presion, se toman en cuenta:

Sensor => Esp32
Gateway y Receptor => heltec wifi lora V3

En receptor se modifica el wifi a usar y el token de Influxdb que debe estar previamente instalado en la maquina a mostrar por grafana
Leer y entender como funciona el codigo, si van a colaborar con sus proyectos tomar en cuenta, si necesitan conexion a internet
usar otro dispositivo que nos pase los datos recopilados al "Gateway", ya que se tienen que conectar al wifi, y ese no tiene conexion a
internet, si van a mandar codigos a mi numero personal porfa, me envian su codigo, los datos que quieren recopilar, y las medidas de los datos
