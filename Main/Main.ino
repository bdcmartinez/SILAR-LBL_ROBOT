#include <Wire.h>               // libreria de comunicacion por I2C
#include <LCD.h>                // libreria para funciones de LCD
#include <LiquidCrystal_I2C.h>  // libreria para LCD por I2C
#include <RTClib.h>             // libraria para el modulo RTC, para medir el tiempo
#include "FS.h"                 // Librerias de para memoria SD
#include "SD.h"
#include "SPI.h"
#include <vector>               // Libreria para declarar vectores



LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);  // DIR, E, RW, RS, D4, D5, D6, D7

RTC_DS3231 rtc;


const int chipSelect = 5;  // Pin CS para la tarjeta SD

//Stepper motor


//Counter of steps in a routine
//Steps of the routine
volatile int X[20];
volatile int Y[20];
int MINUTOS[40];
int SEGUNDOS[40];


// Variables for encoders
int CLK_A = 33;
int DT_A = 32;
int BUTTON_A = 39;
int CLK_B = 35;
int DT_B = 34;
int BUTTON_B = 36;

//Variables for step motors
int DIRX = 26;
int PULX = 25;
int DIRY = 14;
int PULY = 27;
int VEL = 300;

struct Asociacion {//la estructura crea una relación entre cada nombre de archivo y un número
  int identificador;
  String nombreArchivo;
};

// Vector que contiene las asociaciones entre identificadores y nombres de archivo

DateTime startTime;
void startTimeForOut(){
  startTime = rtc.now();                             // Momento en que comienza el período
}

class Files{
  private:
  const char *nombreArchivo = "/pos.txt";
  int num_archivos=0;

  public:

  std::vector<Asociacion> asociaciones;

  String fileSelected = "";

  const char *posFile = "/pos.txt";
  const char *fileName = "/fileName.txt";
  int stepNumber=0;
  volatile int AUX_STEPS_X = 0;
  volatile int AUX_STEPS_Y = 0;
  

    volatile int getAUX_STEPS_X(){
      return AUX_STEPS_X;
    }
    volatile int getAUX_STEPS_Y(){
      return AUX_STEPS_Y;
    }

    void saveNameFile(){
      File file = SD.open(fileName, FILE_WRITE);

      if (file) {
        file.seek(0);
        file.print(fileSelected);
        file.close();
        Serial.println("Contenido del archivo guardado correctamente.");
      } else {
        Serial.println("¡Error al abrir el archivo!");
      }

    }
    String getNameFile(){
       File file = SD.open(fileName);
      if (file) {
        while(file.available()) {
          fileSelected += (char)file.read();
        }
        file.close();
        Serial.println("Configuración leída correctamente.");
        return fileSelected;
      } else {
        Serial.println("Archivo de configuración no encontrado. Usando valores predeterminados.");
      }
    }
    

    void sendFileName(String fileName){
      fileSelected = fileName;
    }

    void selectLastFile(){
      if(getNameFile() == ""){//si no hay un archivo seleccionado, selecciona el último que se guardo
          fileAssociations();
          fileSelected = getFileName(num_archivos-1); // Del número de archivos contados se queda con el último

          processData(fileSelected); 
        }else{//si hay un arhivo seleccionado, obtiene los datos de él
          processData(fileSelected); //recupera los datos del archivo seleccionado

        }
    }

    void fileAssociations(){//se crea una relación de números 1-1 a los nombres de archivos de los programas
      // Abrir el directorio raíz
      File directorio = SD.open("/");
      // Si el directorio se abre correctamente
      num_archivos = 0;
      if (directorio) {
        while (true) {  
          // Abrir el siguiente archivo
          File archivo = directorio.openNextFile();
          Asociacion aux;
          
          // Si no hay más archivos, salir del bucle
          if (!archivo) {
            break;
          }
          
          // Imprimir el nombre del archivo
          //Serial.println(archivo.name());
                          
          // Agregar una nueva asociación al final del vector
          aux = {num_archivos,archivo.name()};
          asociaciones.insert(asociaciones.begin(), aux);

          //asociaciones.push_back({num_archivos, archivo.name()});
          
          // Cerrar el archivo
          archivo.close();
          num_archivos = num_archivos + 1;
        }
        
        // Cerrar el directorio
        directorio.close();

      // Iterar sobre las asociaciones y mostrarlas en la consola
      //for (const auto& asociacion : asociaciones) {
      //  lcd.setCursor(contador_1+1, 0);
      //  lcd.print(asociacion.nombreArchivo);
      //}
      } else {
        // Si no se puede abrir el directorio
        Serial.println("Error al abrir el directorio raíz.");
      }


    } 


    String getStringName(DateTime fechaHora) {
      String nombreArchivo = "";
      // Construir el nombre del archivo usando los componentes de fecha y hora
      nombreArchivo += String(fechaHora.year(), DEC);
      nombreArchivo += "-";
      nombreArchivo += dosDigitos(fechaHora.month());
      nombreArchivo += "-";
      nombreArchivo += dosDigitos(fechaHora.day());
      nombreArchivo += "_";
      nombreArchivo += dosDigitos(fechaHora.hour());
      nombreArchivo += "-";
      nombreArchivo += dosDigitos(fechaHora.minute());
      nombreArchivo += "-";
      nombreArchivo += dosDigitos(fechaHora.second());

      return nombreArchivo;
    }

    String getFileName(int identificador) {
      for (const auto& asociacion : asociaciones) {
        if (asociacion.identificador == identificador) {
          return asociacion.nombreArchivo;
        }
      }
      // Si no se encuentra ninguna asociación para el identificador dado
      return "";
    }
    int getstepNumber(){
      return stepNumber;
    }
    void processData(String nombreArchivo) { 
      File archivo = SD.open("/"+nombreArchivo);

      if (!archivo) {
        Serial.println("Error al abrir el archivo.");
        return;
      }

      Serial.println("Leyendo datos del archivo:");

      // Leer cada línea del archivo
      int contador = 0;
      int aux=0;
      while (archivo.available()) {
        String linea = archivo.readStringUntil('\n'); // Leer una línea del archivo
        // Extraer los números de la línea
        X[contador] = linea.substring(0, linea.indexOf(' ')).toInt();
        linea.remove(0, linea.indexOf(' ') + 1); // Eliminar la parte ya leída de la línea
        Y[contador] = linea.substring(0, linea.indexOf(' ')).toInt();
        linea.remove(0, linea.indexOf(' ') + 1);
        MINUTOS[contador] = linea.substring(0, linea.indexOf(' ')).toInt();
        linea.remove(0, linea.indexOf(' ') + 1);
        SEGUNDOS[contador] = linea.toInt();


      /*
        // Imprimir los datos en la consola
        Serial.print("Columna 1: ");
        Serial.print(X[contador]);
        Serial.print(", Columna 2: ");
        Serial.print(Y[contador]);
        Serial.print(", Columna 3: ");
        Serial.print(MINUTOS[contador]);
        Serial.print(", Columna 4: ");
        Serial.println(SEGUNDOS[contador]);
      */  
        if(X[contador]==0 && Y[contador]==0 && MINUTOS[contador]==0 && SEGUNDOS[contador]==0 && aux==0){
          stepNumber = contador+1;
          aux=1;
        }
        contador = contador + 1;

      }

      archivo.close(); // Cerrar el archivo
    }

    void readPreviousSteps() { //Lee la posición guardada x y de los motores
      Serial.println("Leyendo configuración desde la tarjeta SD...");

      // Abre el archivo en modo de lectura
      File file = SD.open(posFile);
      if (file) {
        // Lee las posiciones desde el archivo
        AUX_STEPS_X = file.parseInt();
        AUX_STEPS_Y = file.parseInt();

        // Cierra el archivo
        file.close();
        Serial.println("Configuración leída correctamente.");
      } else {
        Serial.println("Archivo de configuración no encontrado. Usando valores predeterminados.");
      }
    }

    void saveStep(int x, int y, int minutos, int segundos, int stepNumber){
      X[stepNumber]=x;
      Y[stepNumber]=y;
      MINUTOS[stepNumber]=minutos;
      SEGUNDOS[stepNumber]=segundos;
    }

    void savePos(int AUX_STEPS_X, int AUX_STEPS_Y) { //Guarda la posición x y de los motores

      // Abre el archivo en modo de lectura y escritura
      File file = SD.open(nombreArchivo, FILE_WRITE);
      if (file) {
        // Posiciona el puntero de escritura al principio del archivo
        file.seek(0);

        // Sobrescribe los valores existentes
        file.print(AUX_STEPS_X);
        file.print('\n');
        file.print(AUX_STEPS_Y);

        // Trunca el archivo para eliminar cualquier contenido adicional
        // Cierra y vuelve a abrir el archivo en modo de escritura para truncar
        file.close();

      } else {
        Serial.println("Error al abrir el archivo para guardar configuración.");
      }
    }


    void saveMode(){
      DateTime inicio = rtc.now();
      fileSelected = getStringName(inicio)+".txt";
      
      // Abrir el archivo en modo escritura
      File archivo = SD.open("/"+fileSelected, FILE_WRITE);
      if (archivo) {
        // Escribir cada elemento del arreglo en el archivo
        for (int i = 0; i < sizeof(X) / sizeof(X[0]); i++) {
          archivo.print(X[i]);
          archivo.print(' '); // Nueva línea
          archivo.print(Y[i]);
          archivo.print(' '); // Nueva línea
          archivo.print(MINUTOS[i]);
          archivo.print(' '); // Nueva línea
          archivo.print(SEGUNDOS[i]);
          archivo.print('\n'); // Nueva línea
        }

        archivo.close(); // Cerrar el archivo
        lcd.clear();
        lcd.setCursor(2, 1);
        lcd.print("GUARDADO EXITOSO");
      } else {
        // Si no se pudo abrir el archivo
        lcd.setCursor(2, 1);
        lcd.print("ERROR AL GUARDAR.");
      }
        delay(2000);

        lcd.clear();

    }


    String dosDigitos(int numero) {
      if (numero < 10) {
        return "0" + String(numero);
      } else {
        return String(numero);
      }
    }

};






class Encoder{
  private:
    unsigned long ultimoTiempo = 0;
    unsigned long debounceDelay = 300; // Tiempo de debounce en milisegundos

  public:
    int aux=2;
    bool bol = 0;
    bool aux_submenu = 0;
    volatile int AUX_STEPS_X = 0;
    volatile int AUX_POS_A=0;
    volatile int AUX_STEPS_Y = 0;
    volatile int POS_A = 0;      // variable POS_A con valor inicial de 50 y definida
    volatile int POS_B = 0;
    volatile int AUX_POS_A2 = 0;
    volatile int STEPSX=0;
    volatile int STEPSY=0;
    //Counters for movements of stepper motors

    int AUX_STEPSX;
    int AUX_STEPSY;


    bool PUSH_A=0;
    bool PUSH_B=0;
    volatile int limit_POS_A;
    volatile int limit_POS_B;


    void setToZero(){
      POS_A=0;
      STEPSX=0;
      STEPSY=0;
      POS_B=0;
      AUX_POS_A=0;
    }
    void sendPUSH_A(int value){
      PUSH_A=value;
    }
    void sendPUSH_B(int value){
      PUSH_B=value;
    }
    void sendSTEPSX(volatile int value){
      STEPSX = value;
    }
    void sendSTEPSY(volatile int value){
      STEPSY = value;
    }
    void sendSTEPSY(){

    }

    void restoreData(volatile int value1,volatile int value2,volatile int value3,volatile int value4){
      POS_A=value1;
      POS_B=value2;
      STEPSX=value3;
      STEPSY=value4;
      
    }
    
    int getSTEPSX(){
      return STEPSX;
    }
    int getSTEPSY(){
      return STEPSY;
    }
    bool getPUSH_B(){
      return PUSH_B;
    }
    bool getPUSH_A(){
      return PUSH_A;
    }
    void savePOS_A(volatile int aux){
      POS_A = aux;
    }
    void saveAUX_POS_A(volatile int aux){
      AUX_POS_A = aux;
    }

    void savelimit_POS_A(volatile int aux){
      limit_POS_A = aux;
    }
    void savelimit_POS_B(volatile int aux){
      limit_POS_B = aux;
    }
    volatile int getPOS_A(){
      return POS_A;
    }
    volatile int getPOS_B(){
      return POS_B;
    }
    void eraseValues(){
      bol = 0;
      POS_A = 0;
      AUX_POS_A = 0;   
    }
    bool getBol(){
      return bol;
    }
    volatile int getAUX_POS_A(){
      return AUX_POS_A;

    }

    void encoder1(){
      static unsigned long ultimaInterrupcion = 0;  // variable static con ultimo valor de // tiempo de interrupcion
      unsigned long tiempoInterrupcion = millis();  // variable almacena valor de func. millis

      if (tiempoInterrupcion - ultimaInterrupcion > 5) {  // rutina antirebote desestima  // pulsos menores a 5 mseg.
        if (digitalRead(DT_A) == HIGH)                    // si B es HIGH, sentido horario
        {
          POS_A++;  // incrementa POS_A en 1
          STEPSX = STEPSX + 150;
        } else {    // si B es LOW, senti anti horario
          POS_A--;  // decrementa POS_A en 1
          STEPSX = STEPSX - 150;
        }

      POS_A = min(limit_POS_A,max(0, POS_A));
      STEPSX = min(15000, max(0, STEPSX));  // establece limite inferior de 0 y

      ultimaInterrupcion = tiempoInterrupcion;  // guarda valor actualizado del tiempo
      }     
    

    }
    void encoder2(){
      static unsigned long ultimaInterrupcion = 0;  // variable static con ultimo valor de // tiempo de interrupcion
      unsigned long tiempoInterrupcion = millis();  // variable almacena valor de func. millis

      if (tiempoInterrupcion - ultimaInterrupcion > 5) {  // rutina antirebote desestima // pulsos menores a 5 mseg.
        if (digitalRead(DT_B) == HIGH)                    // si B es HIGH, sentido horario
        {
          POS_B++;  // incrementa POS_A en 1
          STEPSY = STEPSY + 100;
        } else {        // si B es LOW, senti anti horario
          POS_B--;  // decrementa POS_A en 1
          STEPSY = STEPSY - 100;
        }


        POS_B = min(limit_POS_B, max(0, POS_B));  // establece limite inferior de 0 y
        STEPSY = min(10000, max(0, STEPSY));      // establece limite inferior de 0 y

        //savesteps();
        // superior de 100 para POS_A
        ultimaInterrupcion = tiempoInterrupcion;  // guarda valor actualizado del tiempo


      }  // de la interrupcion en variable static

      
    }  

    void push_a(){
      //Variable la cual hará que en determinado menú, nos regresemos al AUX_PRINT_A
      if (millis() - ultimoTiempo > debounceDelay) {
        // Actualiza el tiempo del último cambio del botón
        ultimoTiempo = millis();
        if (digitalRead(BUTTON_B) == LOW) {
          Serial.println("FUNCIONA_B");
          PUSH_B=1;

          // Espera hasta que pase el intervalo
        }
      }
    }

  

    
    void push_b(){

      if (millis() - ultimoTiempo > debounceDelay) {
        // Actualiza el tiempo del último cambio del botón
        ultimoTiempo = millis();
        if (digitalRead(BUTTON_A) == LOW) {
          Serial.println("FUNCIONA_A");
          PUSH_A = 1; 
        }
      }
    }
    
    int max(int num1, int num2) {
      if (num1 > num2) {
        return num1;
      } else {
        return num2;
      }
    }

    int min(int num1, int num2) {
      if (num1 > num2) {
        return num2;
      } else {
        return num1;
      }
    }

                

};

class Values {
  public:
    int STEPSX, STEPSY;
    int POS_A, POS_B;
    
    virtual void saveData(Encoder& EncoderObject){
    }
    virtual void restoreData(Encoder& EncoderObject){

    }
};


class MotorMovement: public Values{ 
  private:
  public:
    volatile int POS_A;
    volatile int POS_B;
    volatile int STEPSX;
    volatile int STEPSY;

    volatile int getPOS_A(){
      return POS_A;
    }
    volatile int getPOS_B(){
      return POS_B;
    }
    volatile int getSTEPSX(){
      return STEPSX;
    }
    volatile int getSTEPSY(){
      return STEPSY;
    }

    void saveData(Encoder& EncoderObject) override{
      POS_A=EncoderObject.getPOS_A();
      POS_B=EncoderObject.getPOS_B();
      STEPSX=EncoderObject.getSTEPSX();
      STEPSY=EncoderObject.getSTEPSY();
    }
    void restoreData(Encoder& EncoderObject) override{
      EncoderObject.restoreData(POS_A,POS_B,STEPSX,STEPSY);
    }
    void moveFromTo(Files& FilesObject,int AUX_STEPS_X, int AUX_STEPS_Y, int STEPSX, int STEPSY) {
      
      
    //Movimiento de motores y

    while (AUX_STEPS_Y < STEPSY) {
      digitalWrite(DIRY, HIGH);

      for (int i = 0; i < 100; i++) {
        digitalWrite(PULY, HIGH);
        delayMicroseconds(VEL);
        digitalWrite(PULY, LOW);
        delayMicroseconds(VEL);
      }

      AUX_STEPS_Y += 100;
      //FilesObject.saveSteps(AUX_STEPS_X,AUX_STEPS_Y);

      


    }

    while (AUX_STEPS_Y > STEPSY) {
    



      digitalWrite(DIRY, LOW);

      for (int i = 0; i < 100; i++) {
        digitalWrite(PULY, HIGH);
        delayMicroseconds(VEL);
        digitalWrite(PULY, LOW);
        delayMicroseconds(VEL);
      }
      AUX_STEPS_Y -= 100;
      //FilesObject.saveSteps(AUX_STEPS_X,AUX_STEPS_Y);


    }
    while (AUX_STEPS_X < STEPSX) {


      digitalWrite(DIRX, HIGH);


      for (int i = 0; i < 150; i++) {
        digitalWrite(PULX, HIGH);
        delayMicroseconds(VEL);
        digitalWrite(PULX, LOW);
        delayMicroseconds(VEL);
      }
      AUX_STEPS_X += 150;
      //FilesObject.saveSteps(AUX_STEPS_X,AUX_STEPS_Y);


    }

    while (AUX_STEPS_X > STEPSX) {
      digitalWrite(DIRX, LOW);

      for (int i = 0; i < 150; i++) {
        digitalWrite(PULX, HIGH);
        delayMicroseconds(VEL);
        digitalWrite(PULX, LOW);
        delayMicroseconds(VEL);
      }

      AUX_STEPS_X -= 150;
      //FilesObject.saveSteps(AUX_STEPS_X,AUX_STEPS_Y);


    }

    FilesObject.savePos(AUX_STEPS_X,AUX_STEPS_Y);

  }


};

//Classes inherited from OptionNavegation

class OptionNavigation{
  public:
    volatile int POS_A;      // variable POS_A con valor inicial de 50 y definida
    volatile int AUX_POS_A;  //almacena el valor de pos_A
    bool aux_PUSH_A;
    bool aux_PUSH_B;
    volatile int OptionNumber; //Me dice cuantas opciones estará desplegando
    volatile int OptionSelection; //Me dirá que opción está eligiendo el usuario
    volatile int currentOption=0;

    bool aux; //Indica que está en ejecución el ciclo


    OptionNavigation() : OptionSelection(-1),aux(1){}

    virtual void Refresh(Encoder& EncoderObject){
      Serial.print("Funciona");
    }

    virtual void Refresh(int i, int j){
      Serial.print("Funciona");
    }
    
    virtual void RefreshTwo(){
      Serial.print("Funciona");
    }

    bool getAUX(){
      return aux;
    }
    void setToZero(Encoder& EncoderObject){
      EncoderObject.setToZero();
      OptionSelection=-1;
      currentOption=0;
      aux=1;
      
    }

    void outForce(Encoder& EncoderObject){
      EncoderObject.setToZero();
      aux = 0;
      EncoderObject.sendPUSH_B(0);
      aux_PUSH_B=0;
      currentOption=0;

      lcd.clear();
    }


    void checkTimeForOut(Encoder& EncoderObject){
      DateTime finalTime = startTime + TimeSpan(0, 0, 1, 0);  // Calcula el momento en que terminará el período
        if (rtc.now() >= finalTime) {
          EncoderObject.setToZero();
          aux = 0;
          EncoderObject.sendPUSH_B(0);
          aux_PUSH_B=0;
          currentOption=0;
          startTimeForOut();
          lcd.clear();
      }
    }


    virtual void out(Encoder& EncoderObject, Files& FilesObject){

    }

    virtual void out(Encoder& EncoderObject){
      aux_PUSH_B = EncoderObject.getPUSH_B();  //Optine el valor

      while (aux_PUSH_B == 1) {
        EncoderObject.setToZero();
        aux = 0;
        EncoderObject.sendPUSH_B(0);
        aux_PUSH_B=0;
        currentOption=0;

        lcd.clear();
        }  
    }
    void buttomState(Encoder& EncoderObject,int predefinedValue=-1){
      aux_PUSH_A = EncoderObject.getPUSH_A();
      while(aux_PUSH_A == 1){
        if(predefinedValue==-1){
        OptionSelection = currentOption;
        }else{
        OptionSelection=predefinedValue;
        }
        EncoderObject.sendPUSH_A(0);
        aux_PUSH_A=0;
        lcd.clear();
      }
    
  }

    String dosDigitos(int numero) {
      if (numero < 10) {
        return "0" + String(numero);
      } else {
        return String(numero);
      }
    }




    void printValues(){
      Serial.println("POS_A: "+String(POS_A)+" AUX_POS_A: "+String(AUX_POS_A));
    }

};

class RefreshRunMode: public OptionNavigation{
  private:
  public:
    DateTime fecha;
    //RefreshRunMode(int option_number) : OptionNavigation(option_number) {}

    void startClock(){
      fecha = rtc.now();                             // Momento en que comienza el período
    }
    void Refresh (int i, int j) override{

      DateTime inicio = rtc.now();                                      // Momento en que comienza el período
      DateTime fin = inicio + TimeSpan(0, 0, MINUTOS[j], SEGUNDOS[j]);  // Calcula el momento en que terminará el período
      while (rtc.now() <= fin) {
        DateTime ahora = rtc.now();
        TimeSpan Transcurrido = ahora - inicio;
        TimeSpan Tiempo_total = ahora - fecha;

        //Impresión del paso en el que va
        lcd.setCursor(7, 1);
        lcd.print(String(j));

        //Impresión del número de la capa en la que va
        lcd.setCursor(16, 1);
        lcd.print(String(i));

        //impresión del tiempo del paso
        lcd.setCursor(9, 2);
        lcd.print(dosDigitos(Transcurrido.minutes()));  // funcion millis() / 1000 para segundos transcurridos
        lcd.print(":");
        lcd.print(dosDigitos(Transcurrido.seconds()));  // funcion millis() / 1000 para segundos transcurridos
        lcd.print("  ");

        //impresión del tiempo total desde el inicio del ciclo
        lcd.setCursor(10, 4);
        lcd.print(dosDigitos(Tiempo_total.minutes()));
        lcd.print(":");
        lcd.print(dosDigitos(Tiempo_total.seconds()));
        lcd.print("  ");

      }
      

      }

    void inter(){
      lcd.setCursor(1, 0);
      lcd.print("*PROCESO EN CURSO*");
      lcd.setCursor(1, 1);
      lcd.print("PASO: ");

      lcd.setCursor(11, 1);
      lcd.print("CAPA: ");

      lcd.setCursor(1, 2);
      lcd.print("T PASO: ");

      lcd.setCursor(1, 4);
      lcd.print("T TOTAL: ");
    }

};



class LineRefresh: public OptionNavigation{
  private:
  public:
    std::vector<Asociacion> palabras;
    int currentPage=0;
    int auxiliar;


    

    //Esta función inserta el listado de opciones
    void OptionNames(const std::vector<Asociacion>& lista_palabras){
      palabras = lista_palabras;
    }

    void lineRefresh(Encoder& EncoderObject){
      POS_A = EncoderObject.getPOS_A();
      AUX_POS_A = EncoderObject.getAUX_POS_A();

      //Este código nos dice en que página estamos del listado de opciones

      std::size_t variable = palabras.size();
      int numeroElementos = static_cast<int>(variable);

      if(POS_A>=20*(currentPage+1)&& POS_A<20*(currentPage+2)){
        currentPage = currentPage+1;
        lcd.clear();
      }else if(POS_A>=20*(currentPage-1) && POS_A<20*currentPage){
        currentPage = currentPage-1;
        lcd.clear();
      }
      Serial.println("CurrentPage:"+String(currentPage));

      if(POS_A!=0){
        if(POS_A<5*(currentOption+2) && POS_A>=5*(currentOption+1)){
          currentOption = currentOption+1;
        }else if(POS_A<5*currentOption && POS_A>=5*(currentOption-1)){
          currentOption = currentOption-1;
        }
      }
      Serial.println("CurrentOption: "+String(currentOption));
      //Serial.println(POS_A);

      int contador = 0;
      int totalPages = numeroElementos/4;
      int limit = currentPage*4+4;
      for (int i = currentPage*4; i < min(limit,numeroElementos) ; i++) {
        lcd.setCursor(1, contador);
        lcd.print(palabras[i].nombreArchivo.substring(0,16)); 
        contador = contador + 1;
        if(contador>4){
          contador=0;
        }
      }

      if(numeroElementos>4){
        if(currentPage==totalPages){
          auxiliar = numeroElementos%4;
          if (auxiliar == 0){
            auxiliar = 4;
          }
        }else{
          auxiliar=4;
        }
      }else if(numeroElementos<4){
        auxiliar=numeroElementos;
      }
      Serial.println(POS_A);

      //Establece la cota superior de POS_A
      EncoderObject.savelimit_POS_A(numeroElementos*5);
  

      switch(auxiliar){

        case 1:
          if (POS_A >= 0 && POS_A <= AUX_POS_A + 5) {
            lcd.setCursor(0, 0);
            lcd.print("-");
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(" ");
          }
          break;
        case 2:
          if (POS_A >= 0 && POS_A < AUX_POS_A + 5) {
              lcd.setCursor(0, 1);
              lcd.print(" ");
              lcd.setCursor(0, 0);
              lcd.print("-");
            }

            if (POS_A >= AUX_POS_A + 5 && POS_A <= AUX_POS_A + 10) {
              lcd.setCursor(0, 0);
              lcd.print(" ");
              lcd.setCursor(0, 1);
              lcd.print("-");
            }


            break;
        case 3:
          if (POS_A >= 0 && POS_A < 5) {
            lcd.setCursor(0, 0);
            lcd.print("-");
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
          }

          if (POS_A >=  5 && POS_A < 10) {
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 1);
            lcd.print("-");
            lcd.setCursor(0, 2);
            lcd.print(" ");

          }

          if (POS_A >= 10 && POS_A <= 15) {
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print("-");
          }

          break;
        case 4:
          if (POS_A >= 0 && POS_A < AUX_POS_A + 5) {
            lcd.setCursor(0, 0);
            lcd.print("-");
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(" ");
          }

          if (POS_A >= AUX_POS_A + 5 && POS_A < AUX_POS_A + 10) {
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 1);
            lcd.print("-");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print(" ");

          }

          if (POS_A >= AUX_POS_A + 10 && POS_A < AUX_POS_A + 15) {
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print("-");
            lcd.setCursor(0, 3);
            lcd.print(" ");
          }
          if (POS_A >= AUX_POS_A + 15 && POS_A < AUX_POS_A + 20) {
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.setCursor(0, 0);
            lcd.print(" ");
            lcd.setCursor(0, 2);
            lcd.print(" ");
            lcd.setCursor(0, 3);
            lcd.print("-");
          }
          if (POS_A >= AUX_POS_A + 20) {
            AUX_POS_A = POS_A;
            EncoderObject.saveAUX_POS_A(AUX_POS_A);
          }
          if (POS_A <= AUX_POS_A - 1) {
            AUX_POS_A = POS_A - 19; 
            EncoderObject.saveAUX_POS_A(AUX_POS_A);

          }
      }  
    }


    int max(int num1, int num2) {
      if (num1 > num2) {
        return num1;
      } else {
        return num2;
      }
    }

    int min(int num1, int num2) {
      if (num1 > num2) {
        return num2;
      } else {
        return num1;
      }
    }

};

class InitialMenu: public LineRefresh{
  public:
    void Refresh(Encoder& EncoderObject) override{ 
      DateTime fecha = rtc.now();
      lcd.setCursor(0, 3);  // ubica cursor en columna 0 y linea 1
      lcd.print("HORA: ");
      lcd.print(dosDigitos(fecha.hour()));  // funcion millis() / 1000 para segundos transcurridos
      lcd.print(":");
      lcd.print(dosDigitos(fecha.minute()));  // funcion millis() / 1000 para segundos transcurridos
      lcd.print(":");
      lcd.print(dosDigitos(fecha.second()));  // funcion millis() / 1000 para segundos transcurridos

    }

};

class RunMode: public OptionNavigation{
  private:
  public:
    int layerNumber;

    //RunMode (int option_number) : OptionNavigation(option_number){}

    void Refresh (Encoder& EncoderObject) override {
        int POS_A=EncoderObject.getPOS_A();
        lcd.setCursor(2, 1);
        lcd.print("NUMERO DE CAPAS:");
        lcd.setCursor(0, 2);
        lcd.print("         ");
        lcd.setCursor(0, 2);
        lcd.print("         ");
        lcd.print(POS_A);
        lcd.print("    ");
    }
    void saveLayerNumber(Encoder& EncoderObject){
        POS_A = EncoderObject.getPOS_A();
        layerNumber = POS_A;
    }
    int getlayerNumber(){
      return layerNumber;
    }



};




class NewModeSteps: public OptionNavigation{
  private:
  public:
    int POS_A;
    int POS_B;
    int stepNumber=0;
  void Refresh(Encoder& EncoderObject) override{
    POS_A = EncoderObject.getPOS_A();
    POS_B = EncoderObject.getPOS_B();

    lcd.setCursor(4, 0);
    lcd.print("*NUEVO MODO*");
    lcd.setCursor(1, 1);
    lcd.print("PASO: ");
    lcd.print(stepNumber);
    lcd.setCursor(1, 2);
    lcd.print("EJEX (0-100): ");
    lcd.print(POS_A);
    lcd.print("  ");
    lcd.setCursor(1, 3);
    lcd.print("EJEY (0-100): ");
    lcd.print(POS_B);
    lcd.print("  ");

  }

  void out(Encoder& EncoderObject, Files& FilesObject) override{
    aux_PUSH_B = EncoderObject.getPUSH_B();  //Optine el valor

    while (aux_PUSH_B == 1) {
      EncoderObject.setToZero();
      aux = 0;
      EncoderObject.sendPUSH_B(0);
      aux_PUSH_B=0;
      currentOption=0;
      
      FilesObject.saveMode();
      FilesObject.saveNameFile();
      lcd.clear();
    }  
  }
};


class NewModeTime: public OptionNavigation{
  public:
    int POS_A;
    int POS_B;
    
    void Refresh(Encoder& EncoderObject) override{
      POS_A = EncoderObject.getPOS_A();
      POS_B = EncoderObject.getPOS_B();

      lcd.setCursor(1, 0);
      lcd.print("*TIEMPO DEL PASO*");
  

      lcd.setCursor(1, 2);
      lcd.print("MINUTOS: ");
      lcd.print(POS_A);
      lcd.print("  ");
      lcd.setCursor(1, 3);
      lcd.print("SEGUNDOS: ");
      lcd.print(POS_B);
      lcd.print("  ");

    }



};


void setup(){
  lcd.setBacklightPin(3, POSITIVE);  // puerto P3 de PCF8574 como positivo
  lcd.setBacklight(HIGH);            // habilita iluminacion posterior de LCD
  lcd.begin(20, 4);                  // 16 columnas por 2 lineas para LCD 1602A
  lcd.clear();    



  pinMode(CLK_A, INPUT_PULLUP);
  pinMode(DT_A, INPUT_PULLUP);
  pinMode(CLK_B, INPUT_PULLUP);
  pinMode(DT_B, INPUT_PULLUP);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  //Pines of stepper motors
  pinMode(DIRX, OUTPUT);
  pinMode(DIRY, OUTPUT);
  pinMode(PULX, OUTPUT);
  pinMode(PULY, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_B), push_a, FALLING);  // interrupcion sobre pin A con
  attachInterrupt(digitalPinToInterrupt(BUTTON_A), push_b, FALLING);  // interrupcion sobre pin A con
  attachInterrupt(digitalPinToInterrupt(CLK_A), encoder1, FALLING);   // interrupcion sobre pin A con
  attachInterrupt(digitalPinToInterrupt(CLK_B), encoder2, FALLING);   // interrupcion sobre pin A con

  if (!rtc.begin()) {
    Serial.println("¡Modulo RTC no encontrado!");
    while (1)
      ;
  }
  if (!SD.begin(chipSelect)) {
    Serial.println("La inicialización de la tarjeta SD falló. Verifique la tarjeta SD en el ESP32 o Arduino.");
    while (1)
      ;
  }
  Serial.begin(115200);


}

Encoder Encoders;


void loop(){
  //Names of the options
  std::vector<Asociacion> c1;
  std::vector<Asociacion> c2;

  LineRefresh Opt_Settings;
  InitialMenu Opt_Inicial;
  RunMode Opt_RunMode;
  RefreshRunMode Opt_RefreshRunMode;

  Files file_OfData;
  MotorMovement Motors;
  LineRefresh Opt_ChangeMode;
  NewModeSteps Opt_NewModeSteps;
  NewModeTime Opt_NewModeTime;



  //alueRefresh valueRefresh(0);
  //OptionNavigation* Opt_RunMode = &valueRefresh;

  c1 = {{0,"INICIAR"},{1,"CONFIGURACION"}};
  Opt_Inicial.OptionNames(c1);
  Opt_Inicial.setToZero(Encoders);
  while(Opt_Inicial.getAUX()){
  switch (Opt_Inicial.OptionSelection){

  case 0: //Opcion Iniciar ciclo
    startTimeForOut();
    Opt_RunMode.setToZero(Encoders);
    while(Opt_RunMode.getAUX()){

      switch(Opt_RunMode.OptionSelection){
        case 0:
          Opt_RefreshRunMode.setToZero(Encoders);
          while(Opt_RefreshRunMode.getAUX()){
            Opt_RefreshRunMode.inter();
            Opt_RefreshRunMode.startClock(); //Empieza a medir el tiempo inicial del proceso

            file_OfData.selectLastFile(); //Hace lo necesario para recopilar los datos del último archivo


            for (int i = 0; i < Opt_RunMode.getlayerNumber(); i++) {
              for (int j = 0; j < file_OfData.getstepNumber(); j++) {
                volatile int STEPS_X = X[j] * 150;
                volatile int STEPS_Y = Y[j] * 100;
                
                file_OfData.readPreviousSteps();
                Motors.moveFromTo(file_OfData,file_OfData.getAUX_STEPS_X(), file_OfData.getAUX_STEPS_Y(), STEPS_X, STEPS_Y);
                Opt_RefreshRunMode.Refresh(i ,j); //Empezará a contar el tiempo

              }
            }
            Opt_RefreshRunMode.out(Encoders);
            Opt_RefreshRunMode.outForce(Encoders);
          }
        Opt_RunMode.OptionSelection=-1;
        
        break;
        default:
          Opt_RunMode.Refresh(Encoders);//Imprime los valores actualizados en la pantalla
          Opt_RunMode.saveLayerNumber(Encoders);//Guarda la opción seleccionada
          Opt_RunMode.buttomState(Encoders);//Verifica si el usuario presiono el encoder
          Opt_RunMode.checkTimeForOut(Encoders);
          Opt_RunMode.out(Encoders); //Configura la opción de salida del while

      }
    }
    Opt_Inicial.OptionSelection=-1;
    Opt_Inicial.currentOption=0;

  break;

  case 1://Menú 2 de configuración
    startTimeForOut();
    Opt_Settings.setToZero(Encoders);
    while(Opt_Settings.getAUX()){
      c2 = {{0,"CAMBIAR MODO"},{1,"MODIFICAR TIEMPOS"},{2,"MODO NUEVO"}};
      switch(Opt_Settings.OptionSelection){
        case 0:
        startTimeForOut();
        Opt_ChangeMode.setToZero(Encoders);
        file_OfData.fileAssociations();
        Opt_ChangeMode.OptionNames(file_OfData.asociaciones);
        while(Opt_ChangeMode.getAUX()){
          switch(Opt_ChangeMode.OptionSelection){
            case 0:
              file_OfData.sendFileName(file_OfData.asociaciones[Opt_ChangeMode.currentOption].nombreArchivo);
              file_OfData.saveNameFile();
              Opt_ChangeMode.OptionSelection = -1;
              Opt_ChangeMode.currentOption = 0;
            break;

            default:
              Opt_ChangeMode.lineRefresh(Encoders);
              Opt_ChangeMode.buttomState(Encoders,0);
              Opt_ChangeMode.checkTimeForOut(Encoders);
              Opt_ChangeMode.out(Encoders);
              
          }
        }
        
        Opt_Settings.OptionSelection=-1;
        Opt_Settings.currentOption=0;

        break;

        case 1:
        break;

        case 2:
        Opt_NewModeSteps.setToZero(Encoders);
        Encoders.savelimit_POS_A(100);
        Encoders.savelimit_POS_B(100);
        while(Opt_NewModeSteps.getAUX()){
          
          switch(Opt_NewModeSteps.OptionSelection){
            //Se ejecuta esta opción en caso de que se presione de nuevo el encoder 1
            case 0:
              //aqui debería guardarse los valores de los encoders previos
              Motors.saveData(Encoders);
              Opt_NewModeTime.setToZero(Encoders);
              Encoders.savelimit_POS_A(1000);
              Encoders.savelimit_POS_B(60);
              while(Opt_NewModeTime.getAUX()){
                switch(Opt_NewModeTime.OptionSelection){
                  case 0:
                    file_OfData.saveStep(Motors.getPOS_A(),Motors.getPOS_B(),Encoders.getPOS_A(),Encoders.getPOS_B(),Opt_NewModeSteps.stepNumber);
                    Opt_NewModeTime.outForce(Encoders);
                    Motors.restoreData(Encoders);
                    Opt_NewModeSteps.stepNumber+=1;
                  break;
                  default:
                    Opt_NewModeTime.Refresh(Encoders);
                    Opt_NewModeTime.buttomState(Encoders);
                    Opt_NewModeTime.out(Encoders); 
                }
              }
              Opt_NewModeSteps.OptionSelection=-1;

              //Encoders.saveSTEPSX(file_OfData.getAUX_STEPS_X());
              //Encoders.saveSTEPSY(file_OfData.getAUX_STEPS_Y());
            break;
            default:
              Opt_NewModeSteps.Refresh(Encoders);
              Opt_NewModeSteps.buttomState(Encoders);
        
              file_OfData.readPreviousSteps();
              Motors.moveFromTo(file_OfData,file_OfData.getAUX_STEPS_X(), file_OfData.getAUX_STEPS_Y(), Encoders.getSTEPSX(), Encoders.getSTEPSY());

              Opt_NewModeSteps.out(Encoders,file_OfData); //Esta salida debería ser especial y que al salir guarde los datos sacas
              //              //Se guardan los valores x y dados por el usuario, esto para formar un paso
              //file_OfData.saveSteps(file_OfData.getAUX_STEPS_X(), file_OfData.getAUX_STEPS_Y()); //Guarda la posición hecha por el usuario

          }
        }
        Opt_Settings.OptionSelection=-1;
        Opt_Settings.currentOption=0;

        break;

        default:


          Opt_Settings.OptionNames(c2);
          Opt_Settings.lineRefresh(Encoders);
          Opt_Settings.buttomState(Encoders);
          Opt_Settings.checkTimeForOut(Encoders);
          Opt_Settings.out(Encoders);


      }
    }
    Opt_Inicial.OptionSelection = -1;
    Opt_Inicial.currentOption = 0;

  break;

  default://Imprime el menú inicial

    //Opt_Inicial.inter();
    Opt_Inicial.Refresh(Encoders);
    Opt_Inicial.lineRefresh(Encoders);
    Opt_Inicial.out(Encoders);
    Opt_Inicial.buttomState(Encoders);
  }

}


}
void encoder1() {
  Encoders.encoder1();
}


void encoder2() {
  Encoders.encoder2();
}


void push_a() {
  Encoders.push_a();

}

void push_b() {
  Encoders.push_b();
}


