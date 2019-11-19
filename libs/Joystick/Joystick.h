/**
 * @author: pettinz
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <linux/joystick.h>

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <exception>
#include <functional>

namespace Politocean
{

class Joystick
{
    const std::string LIB_TAG = "Joystick";
    std::string device_;

    int fd, num_of_axes, num_of_buttons;
    char name_of_joystick[80];
    struct js_event js;

    bool isReading_, isConnected_;

    /**
     * @axes and @buttons maps store respectively values for joystick axes and buttons.
     */
    std::vector<int> axes_;
    unsigned char button_;

    /*
     * Reads data from joystick and stores them.
     */
    void readData();

    std::thread *readingThread_;

    const int SLEEP_TIME = 5; //ms

public:
    static const std::string DFLT_DEVICE;
    /**
     * Opens the joystick file descriptor @fd.
     * It throws a @JoystickException if the open fails.
     */

    Joystick() : Joystick(DFLT_DEVICE) {}

    Joystick(const std::string &device)
        : device_(device), num_of_axes(0), num_of_buttons(0), isReading_(false), isConnected_(false), button_(0), readingThread_(nullptr) {}
    /**
     * Closes the joystick file descriptor @fd.
     */
    ~Joystick();

    void connect();

    /**
     * Returns a thread which is listening to the joystick.
     * @fp is a pointer to the method function
     * @obj is the pointer to the instance object
     */
    template <class M, class T>
    void startReading(void (T::*fp)(const std::vector<int> &axes, unsigned char button), M *obj)
    {
        if (isReading())
            return;

        isReading_ = true;

        auto callbackFunction = std::bind(fp, obj, std::placeholders::_1, std::placeholders::_2);

        readingThread_ = new std::thread([this, callbackFunction]() {
            while (isReading_)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
                readData();
                callbackFunction(axes_, button_);
            }
        });
    }
    /**
     * It stops the listening thread by setting @_isListening to false.
     */
    void stopReading();

    /**
     * Returns the value respectively for the @axis axis and @button button inside the maps.
     */
    int getAxis(int axis);
    unsigned char getButton();

    // Returns true is the thread is reading for joystick values
    bool isReading();
    // Returns true if the joystick device is connected
    bool isConnected();
};

} // namespace Politocean

#endif //JOYSTICK_H
