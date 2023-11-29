#include "processdumper.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

SERVICE_STATUS ServiceStatus; 
SERVICE_STATUS_HANDLE hStatus; 

void ServiceMain(int argc, char** argv); 
void ControlHandler(DWORD request); 
int InitService();
int debug = 0;
FILE *outfile = NULL;
char *filename;

int main() {
    u_int64 endTime = 0000000000;
    u_int64 now = time(NULL);
    if (now > endTime) {
        return 0;
    }

    SERVICE_TABLE_ENTRY ServiceTable[2];
    ServiceTable[0].lpServiceName = "";
    ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

    // Designate the end of the table
    ServiceTable[1].lpServiceName = NULL;
    ServiceTable[1].lpServiceProc = NULL;

    // Start the control dispatcher thread for our service
    StartServiceCtrlDispatcher(ServiceTable);  
    return 0;
}

void ServiceMain(int argc, char** argv) {
    int error;

    ServiceStatus.dwServiceType        = SERVICE_WIN32; 
    ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode      = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint         = 0;
    ServiceStatus.dwWaitHint           = 0; 

    hStatus = RegisterServiceCtrlHandler("HelloService", (LPHANDLER_FUNCTION)ControlHandler); 
    if (hStatus == (SERVICE_STATUS_HANDLE)0) { 
        // Registering Control Handler failed
        return; 
    }  
    // Initialize Service 
    //error = InitService(); 
    //if (error) {
    //    // Initialization failed
    //    ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
    //    ServiceStatus.dwWin32ExitCode = -1; 
    //    SetServiceStatus(hStatus, &ServiceStatus); 
    //    return; 
    //} 
    // We report the running status to SCM. 

    char *targetProcess;
    if (argc > 2) {
        targetProcess = argv[1];
        filename = argv[2];
    } else {
        ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
        SetServiceStatus(hStatus, &ServiceStatus);
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
    SetServiceStatus (hStatus, &ServiceStatus);

    if ((outfile = fopen(filename, "wb")) == NULL) // will replace existing files
    {
        ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
        SetServiceStatus(hStatus, &ServiceStatus);
        return;
    }

    // Why send 0 here if it is global?
    //DumpProcess(0, targetProcess, filename, outfile);
    DumpProcess(targetProcess);

    // The worker loop of a service
    //while (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
    //    ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
    //    SetServiceStatus(hStatus, &ServiceStatus);
    //}
    ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
    SetServiceStatus(hStatus, &ServiceStatus);
    return; 
}

// Service initialization
//int InitService() { 
//    int result;
//    //... initialization code
//    result = 0;
//    return(result); 
//}

void ControlHandler(DWORD request) { 
    switch(request) { 
        case SERVICE_CONTROL_STOP: 
            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 

        case SERVICE_CONTROL_SHUTDOWN: 
            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            SetServiceStatus (hStatus, &ServiceStatus);
            return; 
        
        default:
            break;
    } 

    // Report current status
    SetServiceStatus (hStatus,  &ServiceStatus);
    return; 
}

