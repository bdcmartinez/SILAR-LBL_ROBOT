#include <Wire.h>               // libreria de comunicacion por I2C
#include <LCD.h>                // libreria para funciones de LCD
#include <LiquidCrystal_I2C.h>  // libreria para LCD por I2C
#include <RTClib.h>             // libraria para el modulo RTC, para medir el tiempo
#include "FS.h"                 // Librerias de para memoria SD
#include "SD.h"
#include "SPI.h"
#include <vector>               // Libreria para declarar vectores
#include <TMCStepper.h>
#include <Arduino.h>



LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);  // DIR, E, RW, RS, D4, D5, D6, D7

RTC_DS3231 rtc;


const int chipSelect = 5;  // Pin CS para la tarjeta SD

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
int DIRX = 12;
int PULX = 27;

int DIRY = 25;
int PULY = 26;
int VEL = 300;

int FACTOR_MOVEMENT = 400; // pasos/mm

int GENERAL_STEP_X = 3; //mm
int GENERAL_STEP_Y = 2; //mm

//Variables for drivers
#define RXD2 16
#define TXD2 17

static constexpr float R_SENSE = 0.11f;
static constexpr uint8_t DRIVER_ADDR_1 = 0;
static constexpr uint8_t DRIVER_ADDR_2 = 1;

int current_usteps = 8;

HardwareSerial TMCSerial(2);
TMC2209Stepper driver1(&TMCSerial, R_SENSE, DRIVER_ADDR_1);
TMC2209Stepper driver2(&TMCSerial, R_SENSE, DRIVER_ADDR_2);




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
          STEPSX = STEPSX + GENERAL_STEP_X;
        } else {    // si B es LOW, senti anti horario
          POS_A--;  // decrementa POS_A en 1
          STEPSX = STEPSX - GENERAL_STEP_X;
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
          STEPSY = STEPSY + GENERAL_STEP_Y;
        } else {        // si B es LOW, senti anti horario
          POS_B--;  // decrementa POS_A en 1
          STEPSY = STEPSY - GENERAL_STEP_Y;
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

    void stepMotor_y(int mm, int intervalUs) {
      int steps = mm*FACTOR_MOVEMENT;
      for (int i = 0; i < steps; i++) {
        digitalWrite(PULY, HIGH);
        delayMicroseconds(10);      // pulso seguro
        digitalWrite(PULY, LOW);
        delayMicroseconds(intervalUs);
      
      }
    }

    void stepMotor_x(int mm, int intervalUs) {
      int steps = mm*FACTOR_MOVEMENT;
      for (int i = 0; i < steps; i++) {
        digitalWrite(PULX, HIGH);
        delayMicroseconds(10);      // pulso seguro
        digitalWrite(PULX, LOW);
        delayMicroseconds(intervalUs);
      
      }
    }

    void moveFromTo(Files& FilesObject,int AUX_STEPS_X, int AUX_STEPS_Y, int STEPSX, int STEPSY) {
      //Movimiento de motores y

      while (AUX_STEPS_Y < STEPSY) {
        digitalWrite(DIRY, HIGH);
        stepMotor_y(GENERAL_STEP_Y, 150);
        AUX_STEPS_Y += GENERAL_STEP_Y;

      }

      while (AUX_STEPS_Y > STEPSY) {
        digitalWrite(DIRY, LOW);
        stepMotor_y(GENERAL_STEP_Y, 150);
        AUX_STEPS_Y -= GENERAL_STEP_Y;

      }
      while (AUX_STEPS_X < STEPSX) {
        digitalWrite(DIRX, HIGH);
        stepMotor_x(GENERAL_STEP_X, 150);
        AUX_STEPS_X += GENERAL_STEP_X;

      }

      while (AUX_STEPS_X > STEPSX) {
        digitalWrite(DIRX, LOW);
        stepMotor_x(GENERAL_STEP_X, 150);
        AUX_STEPS_X -= GENERAL_STEP_X;
      }

      FilesObject.savePos(AUX_STEPS_X,AUX_STEPS_Y);
      printDriver1Info("Driver 1 Info");
      printDriver2Info("Driver 2 Info");

  }


};

//------Classes refer to LCD options------

class ILCDBaseNavigation{
  public:
    volatile int POS_A;      // variable POS_A con valor inicial de 50 y definida
    volatile int AUX_POS_A;  //almacena el valor de pos_A
    bool aux_PUSH_A;
    bool aux_PUSH_B;
    volatile int OptionNumber; //Me dice cuantas opciones estará desplegando
    volatile int OptionSelection; //Me dirá que opción está eligiendo el usuario
    volatile int currentOption=0;

    bool aux; //Indica que está en ejecución el ciclo


    ILCDBaseNavigation() : OptionSelection(-1),aux(1){}

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

class LCDRefreshRunMode: public ILCDBaseNavigation{
  private:
  public:
    DateTime fecha;
    //LCDRefreshRunMode(int option_number) : ILCDBaseNavigation(option_number) {}

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

class LCDLineRefresh: public ILCDBaseNavigation{
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

class LCDInitialMenu: public LCDLineRefresh{
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

class LCDRunMode: public ILCDBaseNavigation{
  private:
  public:
    int layerNumber;

    //LCDRunMode (int option_number) : ILCDBaseNavigation(option_number){}

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

class LCDNewModeSteps: public ILCDBaseNavigation{
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

class LCDNewModeTime: public ILCDBaseNavigation{
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

//------Classes refer to LCD options------



void setup(){
  Serial.begin(115200);
  delay(1000);

  lcd.setBacklightPin(3, POSITIVE);  // puerto P3 de PCF8574 como positivo
  lcd.setBacklight(HIGH);            // habilita iluminacion posterior de LCD
  lcd.begin(20, 4);                  // 16 columnas por 2 lineas para LCD 1602A
  lcd.clear();    


  //Encoder Pines 
  pinMode(CLK_A, INPUT_PULLUP);
  pinMode(DT_A, INPUT_PULLUP);
  pinMode(CLK_B, INPUT_PULLUP);
  pinMode(DT_B, INPUT_PULLUP);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  //Stepper motors Pines
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
    while (1);
  }
  if (!SD.begin(chipSelect)) {
    Serial.println("La inicialización de la tarjeta SD falló. Verifique la tarjeta SD en el ESP32 o Arduino.");
    //Impresión del paso en el que va
    lcd.setCursor(1, 1);
    lcd.print("SD card not found!");
    lcd.setCursor(1, 2);
    lcd.print("connect and restart");

    while (1);
  }

  //Drivers configuration
  TMCSerial.begin(115200, SERIAL_8N1, RXD2, TXD2);

  // Driver initialization
  driver1.begin();
  driver2.begin();

  // UART recomendado
  driver1.pdn_disable(true);       // usar PDN_UART como UART
  driver2.pdn_disable(true);       // usar PDN_UART como UART

  driver1.I_scale_analog(false);   // no depender del pot
  driver2.I_scale_analog(false);   // no depender del pot

  // Chopper ON (si toff=0 no mueve)
  driver1.toff(3);
  driver2.toff(3);

  // Corriente: tu motor es 0.7A -> empieza seguro
  driver1.rms_current(500);        // sube/baja luego (600–700 recomendado)
  driver2.rms_current(500);        // sube/baja luego (600–700 recomendado)

  driver1.microsteps(current_usteps);
  driver2.microsteps(current_usteps);

  // Lee una vez
  Serial.print("IFCNT: "); Serial.println(driver1.IFCNT());
  Serial.print("GSTAT: 0x"); Serial.println(driver1.GSTAT(), HEX);
  Serial.print("IOIN: 0x"); Serial.println(driver1.IOIN(), HEX);
  Serial.print("DRV_STATUS: 0x"); Serial.println(driver1.DRV_STATUS(), HEX);
  Serial.print("cs_actual: "); Serial.println(driver1.cs_actual());

  Serial.print("IFCNT: "); Serial.println(driver2.IFCNT());
  Serial.print("GSTAT: 0x"); Serial.println(driver2.GSTAT(), HEX);
  Serial.print("IOIN: 0x"); Serial.println(driver2.IOIN(), HEX);
  Serial.print("DRV_STATUS: 0x"); Serial.println(driver2.DRV_STATUS(), HEX);
  Serial.print("cs_actual: "); Serial.println(driver2.cs_actual());

  Serial.println("=== END SETUP READ ===");

}

Encoder Encoders;


void loop(){
  //Names of the options
  std::vector<Asociacion> c1;
  std::vector<Asociacion> c2;

  LCDLineRefresh LCD_LRSettings;
  LCDLineRefresh LCD_LRChangeMode;

  LCDInitialMenu LCD_StartMenu;
  LCDRunMode LCD_RunMode;
  LCDRefreshRunMode LCD_RefreshRunMode;
  LCDNewModeSteps LCD_NewModeSteps;
  LCDNewModeTime LCD_NewModeTime;


  Files SD_Files;
  MotorMovement Motors;




  //alueRefresh valueRefresh(0);
  //ILCDBaseNavigation* LCD_RunMode = &valueRefresh;

  c1 = {{0,"CORRER PASOS"},{1,"CONFIGURACION"}};
  LCD_StartMenu.OptionNames(c1);
  LCD_StartMenu.setToZero(Encoders);
  while(LCD_StartMenu.getAUX()){
    switch (LCD_StartMenu.OptionSelection){

      case 0: //Opcion Iniciar ciclo
        startTimeForOut();
        LCD_RunMode.setToZero(Encoders);
        while(LCD_RunMode.getAUX()){

          switch(LCD_RunMode.OptionSelection){
            case 0:
              LCD_RefreshRunMode.setToZero(Encoders);
              while(LCD_RefreshRunMode.getAUX()){
                LCD_RefreshRunMode.inter();
                LCD_RefreshRunMode.startClock(); //Empieza a medir el tiempo inicial del proceso

                SD_Files.selectLastFile(); //Hace lo necesario para recopilar los datos del último archivo

                for (int i = 0; i < LCD_RunMode.getlayerNumber(); i++) {
                  for (int j = 0; j < SD_Files.getstepNumber(); j++) {
                    volatile int STEPS_X = X[j] * GENERAL_STEP_X;
                    volatile int STEPS_Y = Y[j] * GENERAL_STEP_Y;
                    
                    SD_Files.readPreviousSteps();
                    Motors.moveFromTo(SD_Files,SD_Files.getAUX_STEPS_X(), SD_Files.getAUX_STEPS_Y(), STEPS_X, STEPS_Y);
                    LCD_RefreshRunMode.Refresh(i ,j); //Empezará a contar el tiempo

                  }
                }
                LCD_RefreshRunMode.out(Encoders);
                LCD_RefreshRunMode.outForce(Encoders);
              }
            LCD_RunMode.OptionSelection=-1;
            
            break;
            default:
              LCD_RunMode.Refresh(Encoders);//Imprime los valores actualizados en la pantalla
              LCD_RunMode.saveLayerNumber(Encoders);//Guarda la opción seleccionada
              LCD_RunMode.buttomState(Encoders);//Verifica si el usuario presiono el encoder
              LCD_RunMode.checkTimeForOut(Encoders);
              LCD_RunMode.out(Encoders); //Configura la opción de salida del while

          }
        }
        LCD_StartMenu.OptionSelection=-1;
        LCD_StartMenu.currentOption=0;

      break;

      case 1://Menú 2 de configuración
        startTimeForOut();
        LCD_LRSettings.setToZero(Encoders);
        while(LCD_LRSettings.getAUX()){
          c2 = {{0,"CAMBIAR MODO"},{1,"MODIFICAR TIEMPOS"},{2,"MODO NUEVO"}};
          switch(LCD_LRSettings.OptionSelection){
            case 0:
              startTimeForOut();
              LCD_LRChangeMode.setToZero(Encoders);
              SD_Files.fileAssociations();
              LCD_LRChangeMode.OptionNames(SD_Files.asociaciones);
              while(LCD_LRChangeMode.getAUX()){
                switch(LCD_LRChangeMode.OptionSelection){
                  case 0:
                    SD_Files.sendFileName(SD_Files.asociaciones[LCD_LRChangeMode.currentOption].nombreArchivo);
                    SD_Files.saveNameFile();
                    LCD_LRChangeMode.OptionSelection = -1;
                    LCD_LRChangeMode.currentOption = 0;
                  break;

                  default:
                    LCD_LRChangeMode.lineRefresh(Encoders);
                    LCD_LRChangeMode.buttomState(Encoders,0);
                    LCD_LRChangeMode.checkTimeForOut(Encoders);
                    LCD_LRChangeMode.out(Encoders);
                    
                }
              }
              
              LCD_LRSettings.OptionSelection=-1;
              LCD_LRSettings.currentOption=0;

            break;

            case 1:
            break;

            case 2:
              LCD_NewModeSteps.setToZero(Encoders);
              Encoders.savelimit_POS_A(100);
              Encoders.savelimit_POS_B(100);
              while(LCD_NewModeSteps.getAUX()){
                
                switch(LCD_NewModeSteps.OptionSelection){
                  //Se ejecuta esta opción en caso de que se presione de nuevo el encoder 1
                  case 0:
                    //aqui debería guardarse los valores de los encoders previos
                    Motors.saveData(Encoders);
                    LCD_NewModeTime.setToZero(Encoders);
                    Encoders.savelimit_POS_A(1000);
                    Encoders.savelimit_POS_B(60);
                    while(LCD_NewModeTime.getAUX()){
                      switch(LCD_NewModeTime.OptionSelection){
                        case 0:
                          SD_Files.saveStep(Motors.getPOS_A(),Motors.getPOS_B(),Encoders.getPOS_A(),Encoders.getPOS_B(),LCD_NewModeSteps.stepNumber);
                          LCD_NewModeTime.outForce(Encoders);
                          Motors.restoreData(Encoders);
                          LCD_NewModeSteps.stepNumber+=1;
                        break;
                        default:
                          LCD_NewModeTime.Refresh(Encoders);
                          LCD_NewModeTime.buttomState(Encoders);
                          LCD_NewModeTime.out(Encoders); 
                      }
                    }
                    LCD_NewModeSteps.OptionSelection=-1;

                    //Encoders.saveSTEPSX(SD_Files.getAUX_STEPS_X());
                    //Encoders.saveSTEPSY(SD_Files.getAUX_STEPS_Y());
                  break;
                  default:
                    LCD_NewModeSteps.Refresh(Encoders);
                    LCD_NewModeSteps.buttomState(Encoders);
              
                    SD_Files.readPreviousSteps();
                    Motors.moveFromTo(SD_Files,SD_Files.getAUX_STEPS_X(), SD_Files.getAUX_STEPS_Y(), Encoders.getSTEPSX(), Encoders.getSTEPSY());

                    LCD_NewModeSteps.out(Encoders,SD_Files); //Esta salida debería ser especial y que al salir guarde los datos sacas
                    //              //Se guardan los valores x y dados por el usuario, esto para formar un paso
                    //SD_Files.saveSteps(SD_Files.getAUX_STEPS_X(), SD_Files.getAUX_STEPS_Y()); //Guarda la posición hecha por el usuario

                }
              }
              LCD_LRSettings.OptionSelection=-1;
              LCD_LRSettings.currentOption=0;

            break;

            default:


              LCD_LRSettings.OptionNames(c2);
              LCD_LRSettings.lineRefresh(Encoders);
              LCD_LRSettings.buttomState(Encoders);
              LCD_LRSettings.checkTimeForOut(Encoders);
              LCD_LRSettings.out(Encoders);


          }
        }
        LCD_StartMenu.OptionSelection = -1;
        LCD_StartMenu.currentOption = 0;

      break;

    default://Imprime el menú inicial

      //LCD_StartMenu.inter();
      LCD_StartMenu.Refresh(Encoders);
      LCD_StartMenu.lineRefresh(Encoders);
      LCD_StartMenu.out(Encoders);
      LCD_StartMenu.buttomState(Encoders);
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

void printDriver1Info(const char* tag) {
  Serial.print("\n==================== "); 
  Serial.print(tag); 
  Serial.println(" ====================");

  // ---------- UART real check (IFCNT should increase on successful write) ----------
  uint8_t if_before = driver1.IFCNT();
  driver1.toff(3);           // write something safe
  delay(50);
  uint8_t if_after = driver1.IFCNT();

  Serial.print("IFCNT before/after: ");
  Serial.print(if_before);
  Serial.print(" -> ");
  Serial.println(if_after);

  Serial.print("IFCNT (read): ");
  Serial.println(driver1.IFCNT());

  // ---------- Core status ----------

  Serial.print("PWM_SCALE: ");
  Serial.println(driver1.PWM_SCALE());

  Serial.print("GSTAT: 0x");
  Serial.println(driver1.GSTAT(), HEX);

  Serial.print("DRV_STATUS: 0x");
  Serial.println(driver1.DRV_STATUS(), HEX);

  Serial.print("IOIN: 0x");
  Serial.println(driver1.IOIN(), HEX);

  // ---------- Motion / sensing ----------
  Serial.print("TSTEP: ");
  Serial.println(driver1.TSTEP());

  Serial.print("SG_RESULT: ");
  Serial.println(driver1.SG_RESULT());

  Serial.print("MSCNT (microstep counter): ");
  Serial.println(driver1.MSCNT());

  Serial.print("MSCURACT (coil currents): 0x");
  Serial.println(driver1.MSCURACT(), HEX);

  // ---------- Current / microsteps ----------
  Serial.print("Current scale (cs_actual): ");
  Serial.println(driver1.cs_actual());

  Serial.print("Microsteps (set): ");
  Serial.println(current_usteps);

  Serial.print("IHOLD_IRUN: 0x");
  Serial.println(driver1.IHOLD_IRUN(), HEX);

  // ---------- Configuration snapshots ----------
  Serial.print("GCONF: 0x");
  Serial.println(driver1.GCONF(), HEX);

  Serial.print("CHOPCONF: 0x");
  Serial.println(driver1.CHOPCONF(), HEX);

  Serial.print("PWMCONF: 0x");
  Serial.println(driver1.PWMCONF(), HEX);

  // ---------- Thresholds (mode switching / stall features) ----------
  Serial.print("TPWMTHRS: ");
  Serial.println(driver1.TPWMTHRS());

  Serial.print("TCOOLTHRS: ");
  Serial.println(driver1.TCOOLTHRS());

  Serial.print("COOLCONF: 0x");
  Serial.println(driver1.COOLCONF(), HEX);

  // ---------- Safety flags (decoded) ----------
  uint8_t otpw = driver1.otpw();
  uint8_t ot   = driver1.ot();
  uint8_t s2ga = driver1.s2ga();
  uint8_t s2gb = driver1.s2gb();
  uint8_t ola  = driver1.ola();
  uint8_t olb  = driver1.olb();

  Serial.print("Temp warning (otpw): ");
  Serial.println(otpw);

  Serial.print("Temp shutdown (ot): ");
  Serial.println(ot);

  Serial.print("Short to GND A (s2ga): ");
  Serial.println(s2ga);

  Serial.print("Short to GND B (s2gb): ");
  Serial.println(s2gb);

  Serial.print("Open load A (ola): ");
  Serial.println(ola);

  Serial.print("Open load B (olb): ");
  Serial.println(olb);

}

void printDriver2Info(const char* tag) {
  Serial.print("\n==================== "); 
  Serial.print(tag); 
  Serial.println(" ====================");

  // ---------- UART real check (IFCNT should increase on successful write) ----------
  uint8_t if_before = driver2.IFCNT();
  driver2.toff(3);           // write something safe
  delay(50);
  uint8_t if_after = driver2.IFCNT();

  Serial.print("IFCNT before/after: ");
  Serial.print(if_before);
  Serial.print(" -> ");
  Serial.println(if_after);

  Serial.print("IFCNT (read): ");
  Serial.println(driver2.IFCNT());

  // ---------- Core status ----------

  Serial.print("PWM_SCALE: ");
  Serial.println(driver2.PWM_SCALE());

  Serial.print("GSTAT: 0x");
  Serial.println(driver2.GSTAT(), HEX);

  Serial.print("DRV_STATUS: 0x");
  Serial.println(driver2.DRV_STATUS(), HEX);

  Serial.print("IOIN: 0x");
  Serial.println(driver2.IOIN(), HEX);

  // ---------- Motion / sensing ----------
  Serial.print("TSTEP: ");
  Serial.println(driver2.TSTEP());

  Serial.print("SG_RESULT: ");
  Serial.println(driver2.SG_RESULT());

  Serial.print("MSCNT (microstep counter): ");
  Serial.println(driver2.MSCNT());

  Serial.print("MSCURACT (coil currents): 0x");
  Serial.println(driver2.MSCURACT(), HEX);

  // ---------- Current / microsteps ----------
  Serial.print("Current scale (cs_actual): ");
  Serial.println(driver2.cs_actual());

  Serial.print("Microsteps (set): ");
  Serial.println(current_usteps);

  Serial.print("IHOLD_IRUN: 0x");
  Serial.println(driver2.IHOLD_IRUN(), HEX);

  // ---------- Configuration snapshots ----------
  Serial.print("GCONF: 0x");
  Serial.println(driver2.GCONF(), HEX);

  Serial.print("CHOPCONF: 0x");
  Serial.println(driver2.CHOPCONF(), HEX);

  Serial.print("PWMCONF: 0x");
  Serial.println(driver2.PWMCONF(), HEX);

  // ---------- Thresholds (mode switching / stall features) ----------
  Serial.print("TPWMTHRS: ");
  Serial.println(driver2.TPWMTHRS());

  Serial.print("TCOOLTHRS: ");
  Serial.println(driver2.TCOOLTHRS());

  Serial.print("COOLCONF: 0x");
  Serial.println(driver2.COOLCONF(), HEX);

  // ---------- Safety flags (decoded) ----------
  uint8_t otpw = driver2.otpw();
  uint8_t ot   = driver2.ot();
  uint8_t s2ga = driver2.s2ga();
  uint8_t s2gb = driver2.s2gb();
  uint8_t ola  = driver2.ola();
  uint8_t olb  = driver2.olb();

  Serial.print("Temp warning (otpw): ");
  Serial.println(otpw);

  Serial.print("Temp shutdown (ot): ");
  Serial.println(ot);

  Serial.print("Short to GND A (s2ga): ");
  Serial.println(s2ga);

  Serial.print("Short to GND B (s2gb): ");
  Serial.println(s2gb);

  Serial.print("Open load A (ola): ");
  Serial.println(ola);

  Serial.print("Open load B (olb): ");
  Serial.println(olb);

}

