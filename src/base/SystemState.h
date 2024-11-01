#ifndef SystemState_h
#define SystemState_h

class SystemState
{
public:
    // flag used to trigger a system reboot
    static bool shouldReboot;
    // flag to pause application Run during Firmware Update
    static bool pauseApplication;
};

#endif