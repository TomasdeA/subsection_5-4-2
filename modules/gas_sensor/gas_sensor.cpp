//=====[Libraries]=============================================================

#include "mbed.h"

#include "gas_sensor.h"

//=====[Declaration of private defines]========================================

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalIn mq2(PE_12);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

//=====[Declarations (prototypes) of private functions]========================

//=====[Implementations of public functions]===================================

void gasSensorInit()
{
    //Para futuras necesidades de inicializacion
}

void gasSensorUpdate()
{
    //Para futuras necesidades de actualizacion
}

bool gasSensorRead()
{
    return mq2;
}

//=====[Implementations of private functions]==================================