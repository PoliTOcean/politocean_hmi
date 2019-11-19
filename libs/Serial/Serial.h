#include <string>
#include <exception>

#include <string.h>
#include <termios.h>

enum class BaudRate
{
    B_9600,
    B_38400,
    B_57600,
    B_115200,
    CUSTOM
};

enum class State
{
    OPEN,
    CLOSE
};

class TTY
{
    static struct termios tty_;

    static void get(int fd);
    static void set(int fd);

public:
    static void configure(int fd);
};

class Serial
{
    int fd_;

    std::string device_;
    BaudRate baudRate_;
    State state_;

public:
    static const BaudRate DFLT_BAUDRATE = BaudRate::B_9600;

    Serial(const std::string &device, BaudRate baudRate) : fd_(-1), device_(device), baudRate_(baudRate), state_(State::CLOSE) {}
    Serial(const std::string &device) : Serial(device, DFLT_BAUDRATE) {}

    ~Serial()
    {
        close();
    }

    void open();
    void close();

    void setBaudRate(BaudRate baudRate);

    int read(std::string &str);
    int readLine(std::string &str);
};

class SerialException : public std::exception
{
    std::string msg_;

public:
    SerialException(const std::string &msg) : msg_(msg) {}

    virtual char const *what() const throw()
    {
        return msg_.c_str();
    }
};