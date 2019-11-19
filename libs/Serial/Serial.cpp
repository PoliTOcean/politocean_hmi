#include <iostream>
#include <string.h>

#include "Serial.hpp"

namespace Unix
{
#include <unistd.h>
#include <fcntl.h>
} // namespace Unix

void Serial::setBaudRate(BaudRate baudRate)
{
    baudRate_ = baudRate;
}

void Serial::open()
{
    std::cout << "Attempting to open COM port \"" << device_ << "\"." << std::endl;

    if (device_.empty())
        throw SerialException("No device to open.");

    if ((fd_ = Unix::open(device_.c_str(), O_RDWR)) < 0)
        throw SerialException("Device \"" + device_ + "\" cannot be opened.");

    TTY::configure(fd_);
}

int Serial::read(std::string &str)
{
    char readBuffer[256];
    memset(&readBuffer, '\0', sizeof(readBuffer));

    int num_bytes = Unix::read(fd_, &readBuffer, sizeof(readBuffer));

    if (num_bytes < 0)
        throw SerialException("An error occurred reading serial.");

    if (num_bytes > 0)
        str = std::string(readBuffer, num_bytes);

    return num_bytes;
}

int Serial::readLine(std::string &str)
{
    char readBuffer;
    memset(&readBuffer, '\0', sizeof(readBuffer));

    std::string readLine;

    int num_line = 0;

    while (readBuffer != '\n')
    {
        int num_bytes = Unix::read(fd_, &readBuffer, sizeof(char));

        if (num_bytes < 0)
            throw SerialException("An error occurred reading serial.");

        readLine += readBuffer;
        num_line++;
    }

    str = readLine;

    return num_line;
}

termios TTY::tty_ = {0};

void Serial::close()
{
    if (Unix::close(fd_) < 0)
        throw SerialException("Cannot close serial port.");
}

void TTY::get(int fd)
{
    memset(&tty_, 0, sizeof tty_);

    if (tcgetattr(fd, &tty_) != 0)
        throw SerialException("An error occurred retrieving serial port attributes.");
}

void TTY::configure(int fd)
{
    get(fd);

    tty_.c_cflag &= ~PARENB;        // Clear parity bit, disabling parity (most common)
    tty_.c_cflag &= ~CSTOPB;        // Clear stop field, only one stop bit used in communication (most common)
    tty_.c_cflag |= CS8;            // 8 bits per byte (most common)
    tty_.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS hardware flow control (most common)
    tty_.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty_.c_lflag &= ~ICANON;
    tty_.c_lflag &= ~ECHO;                                                        // Disable echo
    tty_.c_lflag &= ~ECHOE;                                                       // Disable erasure
    tty_.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
    tty_.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
    tty_.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    tty_.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    tty_.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty_.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS;                                          // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT;                                          // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    tty_.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty_.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty_, B9600);
    cfsetospeed(&tty_, B9600);

    set(fd);
}

void TTY::set(int fd)
{
    tcflush(fd, TCIFLUSH);

    // Save tty settings, also checking for error
    if (tcsetattr(fd, TCSANOW, &tty_) != 0)
        throw SerialException("An error occurred setting serial port attributes.");
}