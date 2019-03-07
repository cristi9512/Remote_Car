
![alt text](https://raw.githubusercontent.com/cristi9512/Remote_Car/master/scheme.png)

Mediul de dezvoltare al aplicatiei a fost AtmelStudio. Prin compilarea proiectelor, acesta genereaza fisiere .hex pe care le-am urcat pe placa folosind HIDBootFlash.

In prima faza a algoritmului realizez toate initializarile necesare: setez pinii de intrare si cei de iesire, initializez comunicarea prin usart si timerele folosite de catre senzorul de distanta. Mai departe, folosind busy-waiting printr-o bucla while infinita, verific distanta returnata de catre senzorul HC SR04 si astept primirea unui caracter de la modulul bluetooth. Daca masina se va afla prea aproape de un obstacol aceasta se va opri si ii va fi permis doar mersul inapoi sau rotirea stanga / dreapta.

Cea mai mare parte dintre problemele intampinate au fost generate de senzoroul ultrasonic HC-SR04.

![alt text](https://raw.githubusercontent.com/cristi9512/Remote_Car/master/hardware.png)
