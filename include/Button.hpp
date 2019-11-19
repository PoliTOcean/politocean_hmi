#include "Reflectable.hpp"
#include "json.hpp"

using namespace Reflectable;

namespace Politocean
{
class Button : public IReflectable
{
    int id_, value_;

public:
    Button(int id, int value) : id_(id), value_(value) {}

    void setId(int id) { id_ = id; }
    int getId() { return id_; }

    void setValue(int value) { value_ = value; }
    int getValue() { return value_; }

    static Button parse(const std::string &stringified)
    {
        int id, value;

        try
        {
            auto j_map = nlohmann::json::parse(stringified);

            id = j_map["id"];
            value = j_map["value"];
        }
        catch (const std::exception &e)
        {
            throw ReflectableParsingException(std::string("An error occurred while parsing button: ") + e.what());
        }
        catch (...)
        {
            throw ReflectableParsingException("An error occurred while parsing button.");
        }
        return Button(id, value);
    }

    std::string stringify() override
    {
        nlohmann::json j_map;
        j_map["id"] = id_;
        j_map["value"] = value_;

        return j_map.dump();
    }

    friend inline bool operator==(const Button &lhs, const Button &rhs) { return lhs.value_ == rhs.value_; }
    friend inline bool operator!=(const Button &lhs, const Button &rhs) { return !(lhs.value_ == rhs.value_); }
};
} // namespace Politocean