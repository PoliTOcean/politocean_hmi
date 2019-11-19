/**
 * @author pettinz
 */

#include <Joystick.h>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <mqttLogger.h>

#include <PolitoceanExceptions.hpp>

namespace Politocean
{

const std::string Joystick::DFLT_DEVICE{"/dev/input/js0"};

void Joystick::connect()
{
    if ((fd = open(device_.c_str(), O_RDONLY)) == -1)
        throw JoystickException("Joystick device not found.");

    ioctl(fd, JSIOCGAXES, &num_of_axes);
    ioctl(fd, JSIOCGBUTTONS, &num_of_buttons);
    ioctl(fd, JSIOCGNAME(80), name_of_joystick);

    // Logging
    std::stringstream info;
    info << "Joystick detected: " << name_of_joystick << "\n\t";
    info << num_of_axes << " axis\n\t";
    info << num_of_buttons << "buttons";
    mqttLogger::getInstance(LIB_TAG).log(logger::CONFIG, info.str());
    // End logging

    axes_.resize(num_of_axes, 0);

    fcntl(fd, F_SETFL, O_NONBLOCK);

    isConnected_ = true;
}

Joystick::~Joystick()
{
    isConnected_ = false;
    close(fd);
}

void Joystick::stopReading()
{
    if (!isReading_)
        return;

    isReading_ = false;
    readingThread_->join();
}

void Joystick::readData()
{
    if ((read(fd, &js, sizeof(struct js_event))) == -1 && errno == ENODEV)
        isConnected_ = false;

    switch (js.type & ~JS_EVENT_INIT)
    {
    case JS_EVENT_AXIS:
        axes_[js.number] = js.value;
        break;
    case JS_EVENT_BUTTON:
        button_ = (js.value << 7) | js.number;
        break;
    }
}

int Joystick::getAxis(int axis)
{
    return axes_[axis];
}

unsigned char Joystick::getButton()
{
    return button_;
}

bool Joystick::isReading()
{
    return isReading_;
}

bool Joystick::isConnected()
{
    return isConnected_;
}

} // namespace Politocean