//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "pc_serial_com.h"

#include "siren.h"
#include "fire_alarm.h"
#include "code.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "event_log.h"

#include <cctype> //to solve  a isdigit() problem
//=====[Declaration of private defines]========================================

//=====[Declaration of private data types]=====================================

typedef enum{
    PC_SERIAL_SET_DATE,
    PC_SERIAL_COMMANDS,
    PC_SERIAL_GET_CODE,
    PC_SERIAL_SAVE_NEW_CODE,
} pcSerialComMode_t;

typedef enum{
    DATE_AND_TIME_START,
    DATE_AND_TIME_READ_YEAR,
    DATE_AND_TIME_READ_MONTH,
    DATE_AND_TIME_READ_DAY,
    DATE_AND_TIME_READ_HOUR,
    DATE_AND_TIME_READ_MINUTE,
    DATE_AND_TIME_READ_SECOND,
    DATE_AND_TIME_DONE,
} dateAndTimeInputState_t;

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromPcSerialCom[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static pcSerialComMode_t pcSerialComMode = PC_SERIAL_COMMANDS;
static dateAndTimeInputState_t dateAndTimeInputState = DATE_AND_TIME_DONE;
static bool codeComplete = false;
static int numberOfCodeChars = 0;
static char dateAndTimerInputBuffer[3]; // Almacena los caracteres de entrada para date and time
char yearBuffer[5]  = "";
char monthBuffer[3] = "";
char dayBuffer[3]   = "";
char hourBuffer[3]  = "";
char minuteBuffer[3]= "";
char secondBuffer[3]= "";

//=====[Declarations (prototypes) of private functions]========================

static void pcSerialComStringRead( char* str, int strLength );

static void pcSerialComSetDateAndTime( char receivedChar );
static void pcSerialComGetCodeUpdate( char receivedChar );
static void pcSerialComSaveNewCodeUpdate( char receivedChar );

static void pcSerialComCommandUpdate( char receivedChar );

static void availableCommands();
static void commandShowCurrentAlarmState();
static void commandShowCurrentGasDetectorState();
static void commandShowCurrentOverTemperatureDetectorState();
static void commandEnterCodeSequence();
static void commandEnterNewCode();
static void commandShowCurrentTemperatureInCelsius();
static void commandShowCurrentTemperatureInFahrenheit();
static void commandSetDateAndTime();
static void commandShowDateAndTime();
static void commandShowStoredEvents();

//=====[Implementations of public functions]===================================

void pcSerialComInit()
{
    availableCommands();
}

char pcSerialComCharRead()
{
    char receivedChar = '\0';
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
    }
    return receivedChar;
}

void pcSerialComStringWrite( const char* str )
{
    uartUsb.write( str, strlen(str) );
}

void pcSerialComUpdate()
{
    char receivedChar = pcSerialComCharRead();
    if( receivedChar != '\0' ) {
        switch ( pcSerialComMode ) {
            
            case PC_SERIAL_SET_DATE:
                pcSerialComSetDateAndTime( receivedChar );
            break;

            case PC_SERIAL_COMMANDS:
                pcSerialComCommandUpdate( receivedChar );
            break;

            case PC_SERIAL_GET_CODE:
                pcSerialComGetCodeUpdate( receivedChar );
            break;

            case PC_SERIAL_SAVE_NEW_CODE:
                pcSerialComSaveNewCodeUpdate( receivedChar );
            break;
            default:
                pcSerialComMode = PC_SERIAL_COMMANDS;
            break;
        }
    }    
}

bool pcSerialComCodeCompleteRead()
{
    return codeComplete;
}

void pcSerialComCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void pcSerialComStringRead( char* str, int strLength )
{
    int strIndex;
    for ( strIndex = 0; strIndex < strLength; strIndex++) {
        
        uartUsb.read( &str[strIndex] , 1 );
        uartUsb.write( &str[strIndex] ,1 ); //eco del mensaje recibido
    }
    str[strLength]='\0';
}


static void pcSerialComSetDateAndTime( char receivedChar )
{
    if (receivedChar != '\0') {
        // Siempre debes recibir caracteres mientras el estado no sea 'Done'
        // Procesa el caracter recibido según el estado actual
        switch (dateAndTimeInputState) {
            case DATE_AND_TIME_START:
                pcSerialComStringWrite("Type four digits for the current year (YYYY): ");
                dateAndTimeInputState = DATE_AND_TIME_READ_YEAR;
                break;
            case DATE_AND_TIME_READ_YEAR:
                // Verifica si el caracter recibido es un dígito válido
                if (isdigit(receivedChar)) {
                    // Agrega el caracter al búfer de entrada si hay espacio
                    if (strlen(dateAndTimerInputBuffer) < 4) {
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer)] = receivedChar;
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer) + 1] = '\0';
                    }

                    // Verifica si se han ingresado cuatro dígitos
                    if (strlen(dateAndTimerInputBuffer) == 4) {
                        int yearValue = atoi(dateAndTimerInputBuffer);
                        if (yearValue >= 1900 && yearValue <= 2099) {
                            // El año se ha ingresado correctamente
                            // Cambia al siguiente estado
                            dateAndTimeInputState = DATE_AND_TIME_READ_MONTH;
                            pcSerialComStringWrite("\r\nType two digits for the current month (01-12): ");
                            // Limpia el búfer de entrada para el próximo campo
                            dateAndTimerInputBuffer[0] = '\0';
                        } else {
                            // El año está fuera del rango, muestra un mensaje de error
                            pcSerialComStringWrite("\r\nInvalid year. Please try again (YYYY): ");
                            // Limpia el búfer de entrada para volver a ingresar el año
                            dateAndTimerInputBuffer[0] = '\0';
                        }
                    }
                } else {
                    // El caracter no es un dígito válido, muestra un mensaje de error
                    pcSerialComStringWrite("\r\nInvalid input. Please try again (YYYY): ");
                    // Limpia el búfer de entrada para volver a ingresar el año
                    dateAndTimerInputBuffer[0] = '\0';
                }
                break;
            case DATE_AND_TIME_READ_MONTH:
                // Verifica si el caracter recibido es un dígito válido
                if (isdigit(receivedChar)) {
                    // Agrega el caracter al búfer de entrada si hay espacio
                    if (strlen(dateAndTimerInputBuffer) < 2) {
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer)] = receivedChar;
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer) + 1] = '\0';
                    }

                    // Verifica si se han ingresado dos dígitos y están en el rango correcto
                    if (strlen(dateAndTimerInputBuffer) == 2) {
                        int monthValue = atoi(dateAndTimerInputBuffer);
                        if (monthValue >= 1 && monthValue <= 12) {
                            // El mes se ha ingresado correctamente
                            // Cambia al siguiente estado
                            dateAndTimeInputState = DATE_AND_TIME_READ_DAY;
                            pcSerialComStringWrite("\r\nType two digits for the current day (01-31): ");
                            // Limpia el búfer de entrada para el próximo campo
                            dateAndTimerInputBuffer[0] = '\0';
                        } else {
                            // El mes está fuera del rango, muestra un mensaje de error
                            pcSerialComStringWrite("\r\nInvalid month. Please try again (01-12): ");
                            // Limpia el búfer de entrada para volver a ingresar el mes
                            dateAndTimerInputBuffer[0] = '\0';
                        }
                    }
                } else {
                    // El caracter no es un dígito válido, muestra un mensaje de error
                    pcSerialComStringWrite("\r\nInvalid input. Please try again (01-12): ");
                    // Limpia el búfer de entrada para volver a ingresar el mes
                    dateAndTimerInputBuffer[0] = '\0';
                }
                break;
            case DATE_AND_TIME_READ_DAY:
                // Verifica si el caracter recibido es un dígito válido
                if (isdigit(receivedChar)) {
                    // Agrega el caracter al búfer de entrada si hay espacio
                    if (strlen(dateAndTimerInputBuffer) < 2) {
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer)] = receivedChar;
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer) + 1] = '\0';
                    }

                    // Verifica si se han ingresado dos dígitos y si están en el rango correcto
                    if (strlen(dateAndTimerInputBuffer) == 2) {
                        int dayValue = atoi(dateAndTimerInputBuffer);
                        if (dayValue >= 1 && dayValue <= 31) {
                            // El día se ha ingresado correctamente
                            // Cambia al siguiente estado
                            dateAndTimeInputState = DATE_AND_TIME_READ_HOUR;
                            pcSerialComStringWrite("\r\nType two digits for the current hour (00-23): ");
                            // Limpia el búfer de entrada para el próximo campo
                            dateAndTimerInputBuffer[0] = '\0';
                        } else {
                            // El día está fuera del rango, muestra un mensaje de error
                            pcSerialComStringWrite("\r\nInvalid day. Please try again (01-31): ");
                            // Limpia el búfer de entrada para volver a ingresar el día
                            dateAndTimerInputBuffer[0] = '\0';
                        }
                    }
                } else {
                    // El caracter no es un dígito válido, muestra un mensaje de error
                    pcSerialComStringWrite("\r\nInvalid input. Please try again (01-31): ");
                    // Limpia el búfer de entrada para volver a ingresar el día
                    dateAndTimerInputBuffer[0] = '\0';
                }
                break;
            case DATE_AND_TIME_READ_HOUR:
                // Verifica si el caracter recibido es un dígito válido
                if (isdigit(receivedChar)) {
                    // Agrega el caracter al búfer de entrada si hay espacio
                    if (strlen(dateAndTimerInputBuffer) < 2) {
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer)] = receivedChar;
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer) + 1] = '\0';
                    }

                    // Verifica si se han ingresado dos dígitos y si están en el rango correcto
                    if (strlen(dateAndTimerInputBuffer) == 2) {
                        int hourValue = atoi(dateAndTimerInputBuffer);
                        if (hourValue >= 0 && hourValue <= 23) {
                            // La hora se ha ingresado correctamente
                            // Cambia al siguiente estado
                            dateAndTimeInputState = DATE_AND_TIME_READ_MINUTE;
                            pcSerialComStringWrite("\r\nType two digits for the current minutes (00-59): ");
                            // Limpia el búfer de entrada para el próximo campo
                            dateAndTimerInputBuffer[0] = '\0';
                        } else {
                            // La hora está fuera del rango, muestra un mensaje de error
                            pcSerialComStringWrite("\r\nInvalid hour. Please try again (00-23): ");
                            // Limpia el búfer de entrada para volver a ingresar la hora
                            dateAndTimerInputBuffer[0] = '\0';
                        }
                    }
                } else {
                    // El caracter no es un dígito válido, muestra un mensaje de error
                    pcSerialComStringWrite("\r\nInvalid input. Please try again (00-23): ");
                    // Limpia el búfer de entrada para volver a ingresar la hora
                    dateAndTimerInputBuffer[0] = '\0';
                }
                break;

            case DATE_AND_TIME_READ_MINUTE:
                // Verifica si el caracter recibido es un dígito válido
                if (isdigit(receivedChar)) {
                    // Agrega el caracter al búfer de entrada si hay espacio
                    if (strlen(dateAndTimerInputBuffer) < 2) {
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer)] = receivedChar;
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer) + 1] = '\0';
                    }

                    // Verifica si se han ingresado dos dígitos y si están en el rango correcto
                    if (strlen(dateAndTimerInputBuffer) == 2) {
                        int minuteValue = atoi(dateAndTimerInputBuffer);
                        if (minuteValue >= 0 && minuteValue <= 59) {
                            // Los minutos se han ingresado correctamente
                            // Cambia al siguiente estado
                            dateAndTimeInputState = DATE_AND_TIME_READ_SECOND;
                            pcSerialComStringWrite("\r\nType two digits for the current seconds (00-59): ");
                            // Limpia el búfer de entrada para el próximo campo
                            dateAndTimerInputBuffer[0] = '\0';
                        } else {
                            // Los minutos están fuera del rango, muestra un mensaje de error
                            pcSerialComStringWrite("\r\nInvalid minutes. Please try again (00-59): ");
                            // Limpia el búfer de entrada para volver a ingresar los minutos
                            dateAndTimerInputBuffer[0] = '\0';
                        }
                    }
                } else {
                    // El caracter no es un dígito válido, muestra un mensaje de error
                    pcSerialComStringWrite("\r\nInvalid input. Please try again (00-59): ");
                    // Limpia el búfer de entrada para volver a ingresar los minutos
                    dateAndTimerInputBuffer[0] = '\0';
                }
                break;
            case DATE_AND_TIME_READ_SECOND:
                // Verifica si el caracter recibido es un dígito válido
                if (isdigit(receivedChar)) {
                    // Agrega el caracter al búfer de entrada si hay espacio
                    if (strlen(dateAndTimerInputBuffer) < 2) {
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer)] = receivedChar;
                        dateAndTimerInputBuffer[strlen(dateAndTimerInputBuffer) + 1] = '\0';
                    }

                    // Verifica si se han ingresado dos dígitos y si están en el rango correcto
                    if (strlen(dateAndTimerInputBuffer) == 2) {
                        int secondValue = atoi(dateAndTimerInputBuffer);
                        if (secondValue >= 0 && secondValue <= 59) {
                            // Los segundos se han ingresado correctamente
                            // Cambia al estado final o realiza la acción necesaria
                            // aquí puedes usar los valores de año, mes, día, hora, minuto y segundo
                            // para configurar la fecha y hora
                            pcSerialComStringWrite("\r\nDate and time has been set\r\n");
                            dateAndTimeWrite(atoi(yearBuffer), atoi(monthBuffer), atoi(dayBuffer),
                                            atoi(hourBuffer), atoi(minuteBuffer), secondValue);
                            dateAndTimeInputState = DATE_AND_TIME_DONE; // Cambia al estado final
                        } else {
                            // Los segundos están fuera del rango, muestra un mensaje de error
                            pcSerialComStringWrite("\r\nInvalid seconds. Please try again (00-59): ");
                            // Limpia el búfer de entrada para volver a ingresar los segundos
                            dateAndTimerInputBuffer[0] = '\0';
                        }
                    }
                } else {
                    // El caracter no es un dígito válido, muestra un mensaje de error
                    pcSerialComStringWrite("\r\nInvalid input. Please try again (00-59): ");
                    // Limpia el búfer de entrada para volver a ingresar los segundos
                    dateAndTimerInputBuffer[0] = '\0';
                }
                break;
                

            case DATE_AND_TIME_DONE:
                // Ya hemos terminado, no hacemos nada más
                break;
        }
    }
}


static void pcSerialComGetCodeUpdate( char receivedChar )
{
    codeSequenceFromPcSerialCom[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
   if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        codeComplete = true;
        numberOfCodeChars = 0;
    } 
}

static void pcSerialComSaveNewCodeUpdate( char receivedChar )
{
    static char newCodeSequence[CODE_NUMBER_OF_KEYS];

    newCodeSequence[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
    if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        numberOfCodeChars = 0;
        codeWrite( newCodeSequence );
        pcSerialComStringWrite( "\r\nNew code configured\r\n\r\n" );
    } 
}

static void pcSerialComCommandUpdate( char receivedChar )
{
    switch (receivedChar) {
        case '1': commandShowCurrentAlarmState(); break;
        case '2': commandShowCurrentGasDetectorState(); break;
        case '3': commandShowCurrentOverTemperatureDetectorState(); break;
        case '4': commandEnterCodeSequence(); break;
        case '5': commandEnterNewCode(); break;
        case 'c': case 'C': commandShowCurrentTemperatureInCelsius(); break;
        case 'f': case 'F': commandShowCurrentTemperatureInFahrenheit(); break;
        case 's': case 'S': commandSetDateAndTime(); break;
        case 't': case 'T': commandShowDateAndTime(); break;
        case 'e': case 'E': commandShowStoredEvents(); break;
        default: availableCommands(); break;
    } 
}

static void availableCommands()
{
    pcSerialComStringWrite( "Available commands:\r\n" );
    pcSerialComStringWrite( "Press '1' to get the alarm state\r\n" );
    pcSerialComStringWrite( "Press '2' to get the gas detector state\r\n" );
    pcSerialComStringWrite( "Press '3' to get the over temperature detector state\r\n" );
    pcSerialComStringWrite( "Press '4' to enter the code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press '5' to enter a new code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n" );
    pcSerialComStringWrite( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n" );
    pcSerialComStringWrite( "Press 's' or 'S' to set the date and time\r\n" );
    pcSerialComStringWrite( "Press 't' or 'T' to get the date and time\r\n" );
    pcSerialComStringWrite( "Press 'e' or 'E' to get the stored events\r\n" );
    pcSerialComStringWrite( "\r\n" );
}

static void commandShowCurrentAlarmState()
{
    if ( sirenStateRead() ) {
        pcSerialComStringWrite( "The alarm is activated\r\n");
    } else {
        pcSerialComStringWrite( "The alarm is not activated\r\n");
    }
}

static void commandShowCurrentGasDetectorState()
{
    if ( gasDetectorStateRead() ) {
        pcSerialComStringWrite( "Gas is being detected\r\n");
    } else {
        pcSerialComStringWrite( "Gas is not being detected\r\n");
    }    
}

static void commandShowCurrentOverTemperatureDetectorState()
{
    if ( overTemperatureDetectorStateRead() ) {
        pcSerialComStringWrite( "Temperature is above the maximum level\r\n");
    } else {
        pcSerialComStringWrite( "Temperature is below the maximum level\r\n");
    }
}

static void commandEnterCodeSequence()
{
    if( sirenStateRead() ) {
        pcSerialComStringWrite( "Please enter the four digits numeric code " );
        pcSerialComStringWrite( "to deactivate the alarm: " );
        pcSerialComMode = PC_SERIAL_GET_CODE;
        codeComplete = false;
        numberOfCodeChars = 0;
    } else {
        pcSerialComStringWrite( "Alarm is not activated.\r\n" );
    }
}

static void commandEnterNewCode()
{
    pcSerialComStringWrite( "Please enter the new four digits numeric code " );
    pcSerialComStringWrite( "to deactivate the alarm: " );
    numberOfCodeChars = 0;
    pcSerialComMode = PC_SERIAL_SAVE_NEW_CODE;

}

static void commandShowCurrentTemperatureInCelsius()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadCelsius() );
    pcSerialComStringWrite( str );  
}

static void commandShowCurrentTemperatureInFahrenheit()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadFahrenheit() );
    pcSerialComStringWrite( str );  
}

static void commandSetDateAndTime()
{
    pcSerialComMode = PC_SERIAL_SAVE_NEW_CODE;
    dateAndTimerInputBuffer[0] = '\0';

    dateAndTimeInputState = DATE_AND_TIME_START;
    pcSerialComStringWrite("\r\n");
}

static void commandShowDateAndTime()
{
    char str[100] = "";
    sprintf ( str, "Date and Time = %s", dateAndTimeRead() );
    pcSerialComStringWrite( str );
    pcSerialComStringWrite("\r\n");
}

static void commandShowStoredEvents()
{
    char str[EVENT_STR_LENGTH] = "";
    int i;
    for (i = 0; i < eventLogNumberOfStoredEvents(); i++) {
        eventLogRead( i, str );
        pcSerialComStringWrite( str );   
        pcSerialComStringWrite( "\r\n" );                    
    }
}