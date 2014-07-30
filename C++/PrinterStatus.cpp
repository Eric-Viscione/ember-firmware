/* 
 * File:   PrinterStatus.cpp
 * Author: Richard Greene
 * 
 * The data structure used to communicate status from the print engine to UI 
 * components.
 *
 * Created on July 28, 2014, 5:25 PM
 */

#include <PrinterStatus.h>
#include <Logger.h>

/// Constructor
PrinterStatus::PrinterStatus() :
_state(PrinterOnState),
_change(NoChange),
_UISubState(NoUISubState),
_isError(false),
_isFatalError(false),
_errorCode(0),
_errno(0),
_errorMessage(""),
_numLayers(0),
_currentLayer(0),
_estimatedSecondsRemaining(0),
_jobName(""),
_temperature(0.0f)
{
}

/// Gets the name of a print engine state machine state
const char* PrinterStatus::GetStateName(PrintEngineState state)
{
    static bool initialized = false;
    static const char* stateNames[MaxPrintEngineState];
    if(!initialized)
    {
        // initialize the array of state names
        stateNames[PrinterOnState] = "PrinterOn";
        stateNames[DoorClosedState] = "DoorClosed";
        stateNames[InitializingState] = "Initializing";
        stateNames[DoorOpenState] = "DoorOpen";
        stateNames[HomingState] = "Homing";
        stateNames[HomeState] = "Home";
        stateNames[IdleState] = "Idle";
        stateNames[PrintSetupState] = "PrintSetup";
        stateNames[MovingToStartPositionState] = "MovingToStartPosition";
        stateNames[ExposingState] = "Exposing";
        stateNames[PrintingState] = "Printing";
        stateNames[PausedState] = "Paused";
        stateNames[SeparatingState] = "Separating";
        stateNames[EndingPrintState] = "EndingPrint";  
        stateNames[ConfirmCancelState] = "ConfirmCancel";
        
        initialized = true;
    }
    
    if(state <= UndefinedPrintEngineState ||
       state >= MaxPrintEngineState)
    {
        LOGGER.HandleError(UNKNOWN_PRINT_ENGINE_STATE, false, NULL, state);
        return "";                                                              
    }
    return stateNames[state];
}